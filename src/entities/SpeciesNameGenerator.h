#pragma once

#include <string>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include "Genome.h"
#include "CreatureType.h"
#include "../environment/BiomeSystem.h"

// Forward declarations
class PlanetTheme;
enum class PlanetPreset;

namespace naming {

/**
 * SpeciesNameGenerator - Procedural species name generation based on creature traits
 *
 * Generates unique names like "MossNewt", "EmberShrike", "ReefManta", "FrostGlider"
 * based on genome traits (color, size, speed, habitat)
 */
class SpeciesNameGenerator {
public:
    SpeciesNameGenerator();
    ~SpeciesNameGenerator() = default;

    /**
     * Generate a species name based on genome traits and creature type
     *
     * @param genome The creature's genome containing traits
     * @param type The creature type (herbivore, carnivore, aquatic, etc.)
     * @return A unique species name like "MossNewt" or "EmberShrike"
     */
    std::string generateName(const Genome& genome, CreatureType type) const;

    /**
     * Generate a name with a specific seed for deterministic output
     * Useful for ensuring the same creature always gets the same name
     */
    std::string generateNameWithSeed(const Genome& genome, CreatureType type, uint32_t seed) const;

    /**
     * Set the random seed for name generation
     */
    void setSeed(uint32_t seed);

    /**
     * Generate a biome-aware species name
     * Incorporates biome-specific prefixes (e.g., "Reef" for coral reef, "Tundra" for cold)
     *
     * @param genome The creature's genome
     * @param type The creature type
     * @param biome The biome where the creature was discovered
     * @return A biome-themed species name
     */
    std::string generateNameWithBiome(const Genome& genome, CreatureType type, BiomeType biome) const;

    /**
     * Generate a planet-themed species name
     * Uses planet theme to flavor names (e.g., alien worlds get more exotic prefixes)
     *
     * @param genome The creature's genome
     * @param type The creature type
     * @param biome The biome where discovered
     * @param theme The planet theme (optional, nullptr for default naming)
     * @return A planet-themed species name
     */
    std::string generateNameWithTheme(const Genome& genome, CreatureType type, BiomeType biome,
                                      const PlanetTheme* theme) const;

    /**
     * Get biome-specific prefix pool
     * Returns prefixes appropriate for the given biome
     */
    const std::vector<std::string>& getBiomePrefixes(BiomeType biome) const;

    /**
     * Get planet preset specific prefixes
     * Returns exotic prefixes for alien world themes
     */
    std::vector<std::string> getThemePrefixes(PlanetPreset preset) const;

private:
    // Word lists organized by archetype

    // Archetype prefixes based on creature characteristics
    // Agile (fast + small)
    std::vector<std::string> m_agilePrefixes;
    // Heavy (slow + large)
    std::vector<std::string> m_heavyPrefixes;
    // Shadow/Nocturnal
    std::vector<std::string> m_shadowPrefixes;
    // Nature/Forest
    std::vector<std::string> m_mossPrefixes;
    // Aquatic/Coastal
    std::vector<std::string> m_coralPrefixes;
    // Sky/Aerial
    std::vector<std::string> m_skyPrefixes;
    // Time/Light
    std::vector<std::string> m_dawnPrefixes;
    // Cold/Ice
    std::vector<std::string> m_frostPrefixes;
    // Fire/Heat
    std::vector<std::string> m_emberPrefixes;
    // Thorny/Defensive
    std::vector<std::string> m_thornPrefixes;
    // Reef/Deep water
    std::vector<std::string> m_reefPrefixes;

    // Locomotion suffixes based on movement style
    std::vector<std::string> m_stalkerSuffixes;  // Ground predators
    std::vector<std::string> m_gliderSuffixes;   // Aerial movement
    std::vector<std::string> m_swimmerSuffixes;  // Aquatic movement
    std::vector<std::string> m_crawlerSuffixes;  // Slow ground movement
    std::vector<std::string> m_hopperSuffixes;   // Jumping creatures
    std::vector<std::string> m_diverSuffixes;    // Diving creatures
    std::vector<std::string> m_soarerSuffixes;   // Soaring/gliding
    std::vector<std::string> m_runnerSuffixes;   // Fast ground movement

    // Species words (animal-inspired base names)
    std::vector<std::string> m_birdSpecies;      // finch, shrike, heron, etc.
    std::vector<std::string> m_fishSpecies;      // manta, pike, eel, etc.
    std::vector<std::string> m_reptileSpecies;   // newt, gecko, skink, etc.
    std::vector<std::string> m_insectSpecies;    // beetle, moth, mantis, etc.
    std::vector<std::string> m_mammalSpecies;    // otter, fox, shrew, etc.

    // Random number generator
    mutable std::mt19937 m_rng;

    // Initialize word lists
    void initializePrefixes();
    void initializeSuffixes();
    void initializeSpeciesWords();

    // Select appropriate word based on traits
    std::string selectPrefix(const Genome& genome, CreatureType type) const;
    std::string selectSuffix(const Genome& genome, CreatureType type) const;
    std::string selectSpeciesWord(const Genome& genome, CreatureType type) const;

    // Trait analysis helpers
    bool isAgile(const Genome& genome) const;
    bool isHeavy(const Genome& genome) const;
    bool isNocturnal(const Genome& genome) const;
    bool isAquatic(CreatureType type) const;
    bool isFlying(CreatureType type) const;
    bool isPredator(CreatureType type) const;

    // Generate deterministic random value from genome
    float genomeHash(const Genome& genome, int index) const;
};

// Global instance accessor
SpeciesNameGenerator& getNameGenerator();

} // namespace naming
