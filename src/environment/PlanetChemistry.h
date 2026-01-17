#pragma once

#include <cstdint>
#include <string>
#include <array>

// ============================================================================
// PLANET CHEMISTRY SYSTEM
// ============================================================================
// Defines the fundamental chemistry of a planet that affects all life forms.
// Each planet has unique chemical constraints that drive evolution and create
// selective pressure for biochemical adaptations.

// ============================================================================
// SOLVENT TYPES
// ============================================================================
// The primary liquid medium for life - affects temperature ranges and biochemistry

enum class SolventType : uint8_t {
    WATER = 0,      // Standard Earth-like (liquid range: 0-100°C at 1 atm)
    AMMONIA = 1,    // Cold worlds (liquid range: -78 to -33°C at 1 atm)
    METHANE = 2,    // Very cold worlds (liquid range: -182 to -161°C at 1 atm)
    SULFURIC_ACID = 3, // Hot acidic worlds (Venus-like)
    ETHANOL = 4,    // Moderate temperature range alternative
    COUNT = 5
};

// Get human-readable name for solvent type
inline const char* getSolventName(SolventType type) {
    switch (type) {
        case SolventType::WATER: return "Water";
        case SolventType::AMMONIA: return "Ammonia";
        case SolventType::METHANE: return "Methane";
        case SolventType::SULFURIC_ACID: return "Sulfuric Acid";
        case SolventType::ETHANOL: return "Ethanol";
        default: return "Unknown";
    }
}

// ============================================================================
// ATMOSPHERE COMPOSITION
// ============================================================================
// Normalized gas fractions (sum to 1.0)

struct AtmosphereMix {
    float oxygen;       // 0.0 - 0.4 (Earth is ~0.21)
    float nitrogen;     // 0.0 - 0.9 (Earth is ~0.78)
    float carbonDioxide;// 0.0 - 0.95 (Venus is ~0.96)
    float methane;      // 0.0 - 0.1 (trace on Earth)
    float argon;        // 0.0 - 0.05 (Earth is ~0.01)
    float hydrogen;     // 0.0 - 0.95 (gas giants)
    float helium;       // 0.0 - 0.3 (gas giants)
    float sulfurDioxide;// 0.0 - 0.1 (volcanic worlds)

    // Atmospheric pressure in Earth atmospheres (1.0 = Earth sea level)
    float pressure;     // 0.01 - 100.0 atm

    // Normalize gas fractions to sum to 1.0
    void normalize() {
        float total = oxygen + nitrogen + carbonDioxide + methane +
                      argon + hydrogen + helium + sulfurDioxide;
        if (total > 0.001f) {
            oxygen /= total;
            nitrogen /= total;
            carbonDioxide /= total;
            methane /= total;
            argon /= total;
            hydrogen /= total;
            helium /= total;
            sulfurDioxide /= total;
        }
    }

    // Check if atmosphere is breathable for oxygen-based life
    bool isOxygenBreathable() const {
        return oxygen >= 0.15f && oxygen <= 0.35f &&
               carbonDioxide < 0.05f && pressure >= 0.5f && pressure <= 2.0f;
    }
};

// ============================================================================
// MINERAL PROFILE
// ============================================================================
// Abundance of key elements affecting creature development

struct MineralProfile {
    float iron;         // 0.0 - 1.0 (affects hemoglobin, magnetite formation)
    float silicon;      // 0.0 - 1.0 (affects exoskeletons, silicate structures)
    float calcium;      // 0.0 - 1.0 (affects bones, shells, coral)
    float sulfur;       // 0.0 - 1.0 (affects chemosynthesis, volcanic life)
    float phosphorus;   // 0.0 - 1.0 (affects DNA/RNA, ATP)
    float copper;       // 0.0 - 1.0 (affects hemocyanin, blue blood)
    float magnesium;    // 0.0 - 1.0 (affects chlorophyll, enzymes)
    float zinc;         // 0.0 - 1.0 (affects enzymes, immune function)

    // Get the dominant mineral (highest abundance)
    const char* getDominantMineral() const {
        float maxVal = iron;
        const char* dominant = "Iron";

        if (silicon > maxVal) { maxVal = silicon; dominant = "Silicon"; }
        if (calcium > maxVal) { maxVal = calcium; dominant = "Calcium"; }
        if (sulfur > maxVal) { maxVal = sulfur; dominant = "Sulfur"; }
        if (phosphorus > maxVal) { maxVal = phosphorus; dominant = "Phosphorus"; }
        if (copper > maxVal) { maxVal = copper; dominant = "Copper"; }
        if (magnesium > maxVal) { maxVal = magnesium; dominant = "Magnesium"; }
        if (zinc > maxVal) { dominant = "Zinc"; }

        return dominant;
    }
};

// ============================================================================
// PLANET CHEMISTRY - Complete planetary chemical profile
// ============================================================================

struct PlanetChemistry {
    // ==========================================
    // Primary chemical characteristics
    // ==========================================

    SolventType solventType;      // Primary life-supporting liquid
    AtmosphereMix atmosphere;     // Atmospheric composition
    MineralProfile minerals;      // Mineral abundance

    // ==========================================
    // Environmental chemistry factors
    // ==========================================

    float radiationLevel;         // 0.0 - 2.0 (1.0 = Earth-like UV/cosmic ray exposure)
    float acidity;                // 0.0 - 14.0 (pH scale, 7.0 = neutral)
    float salinity;               // 0.0 - 1.0 (salt concentration in water bodies)
    float temperatureBase;        // -200 to 500 °C (average surface temperature)
    float temperatureRange;       // 0 - 200 °C (variation between day/night, seasons)

    // ==========================================
    // Rare element availability
    // ==========================================

    float rareEarthAbundance;     // 0.0 - 1.0 (lanthanides, affect pigments)
    float radioactiveAbundance;   // 0.0 - 1.0 (uranium, thorium - geothermal)
    float organicComplexity;      // 0.0 - 1.0 (pre-biotic organic molecules)

    // ==========================================
    // Seed used for generation (for reproducibility)
    // ==========================================
    uint32_t generationSeed;

    // ==========================================
    // Initialization and generation
    // ==========================================

    // Default constructor - Earth-like defaults
    PlanetChemistry();

    // Generate chemistry from seed
    static PlanetChemistry fromSeed(uint32_t seed);

    // Generate specific world type chemistries
    static PlanetChemistry earthLike(uint32_t seed);
    static PlanetChemistry volcanicWorld(uint32_t seed);
    static PlanetChemistry frozenWorld(uint32_t seed);
    static PlanetChemistry toxicWorld(uint32_t seed);
    static PlanetChemistry oceanWorld(uint32_t seed);
    static PlanetChemistry desertWorld(uint32_t seed);

    // ==========================================
    // Utility methods
    // ==========================================

    // Get a descriptive name for this chemistry profile
    std::string getProfileName() const;

    // Check if chemistry is hospitable for carbon-based life
    bool isCarbonFriendly() const;

    // Get the optimal temperature range for native life
    void getLifeTemperatureRange(float& minTemp, float& maxTemp) const;

    // Get toxicity level for Earth-like organisms (0 = safe, 1 = lethal)
    float getEarthLifeToxicity() const;
};

// ============================================================================
// CHEMISTRY VARIATION - Seed-driven parameter derivation
// ============================================================================

namespace ChemistryVariation {
    // Sub-seed offsets for chemistry generation
    constexpr uint32_t SOLVENT_OFFSET = 0;
    constexpr uint32_t ATMOSPHERE_OFFSET = 1;
    constexpr uint32_t MINERALS_OFFSET = 2;
    constexpr uint32_t RADIATION_OFFSET = 3;
    constexpr uint32_t ACIDITY_OFFSET = 4;
    constexpr uint32_t SALINITY_OFFSET = 5;
    constexpr uint32_t TEMPERATURE_OFFSET = 6;
    constexpr uint32_t RARE_ELEMENTS_OFFSET = 7;
}
