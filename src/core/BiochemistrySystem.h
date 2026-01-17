#pragma once

#include <cstdint>
#include <unordered_map>
#include <glm/glm.hpp>

// Forward declarations
class Genome;
struct PlanetChemistry;

// ============================================================================
// BIOCHEMISTRY SYSTEM
// ============================================================================
// Computes compatibility between creatures' genomes and the planet's chemistry.
// Creatures with poor compatibility suffer fitness/energy penalties, creating
// selective pressure for biochemical adaptation.

// ============================================================================
// COMPATIBILITY RESULT
// ============================================================================

struct BiochemistryCompatibility {
    // Overall compatibility score (0.0 = lethal, 1.0 = perfect adaptation)
    float overall;

    // Component scores for debugging/UI
    float solventCompatibility;      // How well chemistry works with planet's solvent
    float oxygenCompatibility;       // Oxygen level matching
    float temperatureCompatibility;  // Temperature range overlap
    float radiationCompatibility;    // Radiation resistance vs exposure
    float acidityCompatibility;      // pH tolerance vs environment
    float mineralCompatibility;      // Mineral availability vs needs

    // Derived penalties for gameplay
    float energyPenaltyMultiplier;   // 1.0 = normal, >1.0 = faster energy drain
    float healthPenaltyRate;         // Health loss per second (0 = none)
    float reproductionPenalty;       // Multiplier on reproduction chance (1.0 = normal)

    // Default constructor - perfect compatibility
    BiochemistryCompatibility()
        : overall(1.0f)
        , solventCompatibility(1.0f)
        , oxygenCompatibility(1.0f)
        , temperatureCompatibility(1.0f)
        , radiationCompatibility(1.0f)
        , acidityCompatibility(1.0f)
        , mineralCompatibility(1.0f)
        , energyPenaltyMultiplier(1.0f)
        , healthPenaltyRate(0.0f)
        , reproductionPenalty(1.0f)
    {}
};

// ============================================================================
// PIGMENT HINT - Color suggestions based on biochemistry
// ============================================================================

struct PigmentHint {
    glm::vec3 primaryColor;      // Suggested primary color based on pigment family
    glm::vec3 secondaryColor;    // Secondary accent color
    float saturationBias;        // How saturated colors should be (-1 to 1)
    float brightnessBias;        // How bright colors should be (-1 to 1)

    // Get a blended color incorporating the hint with existing genome color
    glm::vec3 blendWithGenome(const glm::vec3& genomeColor, float strength = 0.3f) const;
};

// ============================================================================
// SPECIES AFFINITY CACHE ENTRY
// ============================================================================

struct SpeciesAffinity {
    uint32_t speciesId;
    BiochemistryCompatibility compatibility;
    PigmentHint pigmentHint;
    uint64_t computedFrame;      // Frame number when computed (for cache invalidation)
    bool isValid;
};

// ============================================================================
// BIOCHEMISTRY SYSTEM CLASS
// ============================================================================

class BiochemistrySystem {
public:
    // Singleton access
    static BiochemistrySystem& getInstance();

    // ==========================================
    // Core computation methods
    // ==========================================

    // Compute compatibility between a genome and planet chemistry
    BiochemistryCompatibility computeCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;

    // Compute pigment color hints based on genome and chemistry
    PigmentHint computePigmentHint(const Genome& genome, const PlanetChemistry& chemistry) const;

    // ==========================================
    // Species-level caching (for performance)
    // ==========================================

    // Get cached affinity for a species (computes if not cached)
    const SpeciesAffinity& getSpeciesAffinity(uint32_t speciesId, const Genome& representativeGenome, const PlanetChemistry& chemistry);

    // Invalidate cache for a species (call when species genome changes significantly)
    void invalidateSpeciesCache(uint32_t speciesId);

    // Clear all caches (call on world reset)
    void clearAllCaches();

    // ==========================================
    // Diagnostic methods
    // ==========================================

    // Get average compatibility across all cached species
    float getAverageCompatibility() const;

    // Get minimum compatibility across all cached species
    float getMinimumCompatibility() const;

    // Get count of species in each compatibility tier
    void getCompatibilityDistribution(int& lethal, int& poor, int& moderate, int& good, int& excellent) const;

    // Log compatibility statistics
    void logStatistics() const;

    // ==========================================
    // Configuration
    // ==========================================

    // Set the current frame number for cache validation
    void setCurrentFrame(uint64_t frame) { m_currentFrame = frame; }

    // Set cache lifetime in frames
    void setCacheLifetime(uint64_t frames) { m_cacheLifetimeFrames = frames; }

private:
    BiochemistrySystem();
    ~BiochemistrySystem() = default;

    // Non-copyable
    BiochemistrySystem(const BiochemistrySystem&) = delete;
    BiochemistrySystem& operator=(const BiochemistrySystem&) = delete;

    // ==========================================
    // Internal computation helpers
    // ==========================================

    // Compute individual compatibility components
    float computeSolventCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;
    float computeOxygenCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;
    float computeTemperatureCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;
    float computeRadiationCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;
    float computeAcidityCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;
    float computeMineralCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const;

    // Convert compatibility to gameplay penalties
    void computePenalties(BiochemistryCompatibility& compat) const;

    // Get pigment color for a pigment family
    glm::vec3 getPigmentFamilyColor(uint8_t pigmentFamily) const;

    // ==========================================
    // Cache storage
    // ==========================================

    std::unordered_map<uint32_t, SpeciesAffinity> m_speciesCache;
    uint64_t m_currentFrame;
    uint64_t m_cacheLifetimeFrames;

    // ==========================================
    // Constants for compatibility calculation
    // ==========================================

    // Weight factors for overall compatibility
    static constexpr float SOLVENT_WEIGHT = 0.25f;
    static constexpr float OXYGEN_WEIGHT = 0.20f;
    static constexpr float TEMPERATURE_WEIGHT = 0.20f;
    static constexpr float RADIATION_WEIGHT = 0.10f;
    static constexpr float ACIDITY_WEIGHT = 0.15f;
    static constexpr float MINERAL_WEIGHT = 0.10f;

    // Penalty thresholds
    static constexpr float LETHAL_THRESHOLD = 0.2f;          // Below this = rapid health loss
    static constexpr float POOR_THRESHOLD = 0.4f;            // Below this = significant penalties
    static constexpr float MODERATE_THRESHOLD = 0.6f;        // Below this = minor penalties
    static constexpr float GOOD_THRESHOLD = 0.8f;            // Above this = minimal penalties
    // Above 0.8 = excellent, no penalties
};

// ============================================================================
// CONVENIENCE MACROS FOR HANDOFF
// ============================================================================
// These provide a simple interface for Agent 10 (Creature.cpp) to integrate
// biochemistry penalties without needing to understand the full system.

// Get energy penalty multiplier for a creature
// Usage: float penalty = BIOCHEM_ENERGY_PENALTY(creature.genome, world.chemistry);
#define BIOCHEM_ENERGY_PENALTY(genome, chemistry) \
    BiochemistrySystem::getInstance().computeCompatibility(genome, chemistry).energyPenaltyMultiplier

// Get health penalty rate for a creature
// Usage: float healthLoss = BIOCHEM_HEALTH_PENALTY(creature.genome, world.chemistry);
#define BIOCHEM_HEALTH_PENALTY(genome, chemistry) \
    BiochemistrySystem::getInstance().computeCompatibility(genome, chemistry).healthPenaltyRate

// Get reproduction penalty for a creature
// Usage: float reproPenalty = BIOCHEM_REPRO_PENALTY(creature.genome, world.chemistry);
#define BIOCHEM_REPRO_PENALTY(genome, chemistry) \
    BiochemistrySystem::getInstance().computeCompatibility(genome, chemistry).reproductionPenalty
