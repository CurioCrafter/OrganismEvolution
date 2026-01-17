#include "PlanetChemistry.h"
#include "PlanetSeed.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// PLANET CHEMISTRY IMPLEMENTATION
// ============================================================================

PlanetChemistry::PlanetChemistry() {
    // Earth-like defaults
    solventType = SolventType::WATER;

    atmosphere.oxygen = 0.21f;
    atmosphere.nitrogen = 0.78f;
    atmosphere.carbonDioxide = 0.0004f;
    atmosphere.methane = 0.00017f;
    atmosphere.argon = 0.0093f;
    atmosphere.hydrogen = 0.0f;
    atmosphere.helium = 0.0f;
    atmosphere.sulfurDioxide = 0.0f;
    atmosphere.pressure = 1.0f;

    minerals.iron = 0.5f;
    minerals.silicon = 0.6f;
    minerals.calcium = 0.4f;
    minerals.sulfur = 0.2f;
    minerals.phosphorus = 0.3f;
    minerals.copper = 0.15f;
    minerals.magnesium = 0.35f;
    minerals.zinc = 0.1f;

    radiationLevel = 1.0f;
    acidity = 7.0f;
    salinity = 0.035f;
    temperatureBase = 15.0f;
    temperatureRange = 50.0f;

    rareEarthAbundance = 0.3f;
    radioactiveAbundance = 0.1f;
    organicComplexity = 0.5f;

    generationSeed = 0;
}

PlanetChemistry PlanetChemistry::fromSeed(uint32_t seed) {
    PlanetChemistry chem;
    chem.generationSeed = seed;

    // Determine world type based on seed distribution
    // 40% Earth-like, 15% Ocean, 15% Desert, 10% Frozen, 10% Volcanic, 10% Toxic
    float typeRoll = PlanetSeed::seedToFloat(seed);

    if (typeRoll < 0.40f) {
        chem = earthLike(seed);
    } else if (typeRoll < 0.55f) {
        chem = oceanWorld(seed);
    } else if (typeRoll < 0.70f) {
        chem = desertWorld(seed);
    } else if (typeRoll < 0.80f) {
        chem = frozenWorld(seed);
    } else if (typeRoll < 0.90f) {
        chem = volcanicWorld(seed);
    } else {
        chem = toxicWorld(seed);
    }

    chem.generationSeed = seed;
    return chem;
}

PlanetChemistry PlanetChemistry::earthLike(uint32_t seed) {
    PlanetChemistry chem;

    chem.solventType = SolventType::WATER;

    // Atmosphere with variation
    uint32_t atmSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::ATMOSPHERE_OFFSET);
    chem.atmosphere.oxygen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 0), 0.18f, 0.25f);
    chem.atmosphere.nitrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 1), 0.72f, 0.82f);
    chem.atmosphere.carbonDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 2), 0.0002f, 0.002f);
    chem.atmosphere.methane = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 3), 0.0f, 0.001f);
    chem.atmosphere.argon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 4), 0.005f, 0.015f);
    chem.atmosphere.hydrogen = 0.0f;
    chem.atmosphere.helium = 0.0f;
    chem.atmosphere.sulfurDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 5), 0.0f, 0.0001f);
    chem.atmosphere.pressure = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 6), 0.8f, 1.3f);
    chem.atmosphere.normalize();

    // Minerals with Earth-like distribution
    uint32_t minSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::MINERALS_OFFSET);
    chem.minerals.iron = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 0), 0.4f, 0.7f);
    chem.minerals.silicon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 1), 0.5f, 0.8f);
    chem.minerals.calcium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 2), 0.3f, 0.6f);
    chem.minerals.sulfur = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 3), 0.1f, 0.3f);
    chem.minerals.phosphorus = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 4), 0.2f, 0.4f);
    chem.minerals.copper = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 5), 0.1f, 0.25f);
    chem.minerals.magnesium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 6), 0.25f, 0.5f);
    chem.minerals.zinc = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 7), 0.05f, 0.15f);

    // Environmental factors
    chem.radiationLevel = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::RADIATION_OFFSET), 0.8f, 1.3f);
    chem.acidity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::ACIDITY_OFFSET), 6.5f, 8.0f);
    chem.salinity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::SALINITY_OFFSET), 0.02f, 0.05f);

    uint32_t tempSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::TEMPERATURE_OFFSET);
    chem.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 0), 10.0f, 25.0f);
    chem.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 1), 30.0f, 70.0f);

    uint32_t rareSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::RARE_ELEMENTS_OFFSET);
    chem.rareEarthAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 0), 0.2f, 0.5f);
    chem.radioactiveAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 1), 0.05f, 0.2f);
    chem.organicComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 2), 0.4f, 0.8f);

    return chem;
}

PlanetChemistry PlanetChemistry::volcanicWorld(uint32_t seed) {
    PlanetChemistry chem;

    chem.solventType = SolventType::SULFURIC_ACID;

    // High CO2, sulfur compounds atmosphere
    uint32_t atmSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::ATMOSPHERE_OFFSET);
    chem.atmosphere.oxygen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 0), 0.01f, 0.08f);
    chem.atmosphere.nitrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 1), 0.1f, 0.3f);
    chem.atmosphere.carbonDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 2), 0.5f, 0.85f);
    chem.atmosphere.methane = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 3), 0.0f, 0.02f);
    chem.atmosphere.argon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 4), 0.01f, 0.05f);
    chem.atmosphere.hydrogen = 0.0f;
    chem.atmosphere.helium = 0.0f;
    chem.atmosphere.sulfurDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 5), 0.02f, 0.1f);
    chem.atmosphere.pressure = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 6), 5.0f, 50.0f);
    chem.atmosphere.normalize();

    // High sulfur and iron minerals
    uint32_t minSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::MINERALS_OFFSET);
    chem.minerals.iron = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 0), 0.7f, 1.0f);
    chem.minerals.silicon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 1), 0.6f, 0.9f);
    chem.minerals.calcium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 2), 0.2f, 0.4f);
    chem.minerals.sulfur = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 3), 0.7f, 1.0f);
    chem.minerals.phosphorus = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 4), 0.1f, 0.3f);
    chem.minerals.copper = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 5), 0.3f, 0.6f);
    chem.minerals.magnesium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 6), 0.4f, 0.7f);
    chem.minerals.zinc = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 7), 0.1f, 0.3f);

    // Hot, acidic environment
    chem.radiationLevel = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::RADIATION_OFFSET), 1.2f, 1.8f);
    chem.acidity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::ACIDITY_OFFSET), 1.0f, 4.0f);
    chem.salinity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::SALINITY_OFFSET), 0.1f, 0.3f);

    uint32_t tempSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::TEMPERATURE_OFFSET);
    chem.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 0), 150.0f, 400.0f);
    chem.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 1), 50.0f, 150.0f);

    uint32_t rareSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::RARE_ELEMENTS_OFFSET);
    chem.rareEarthAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 0), 0.4f, 0.8f);
    chem.radioactiveAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 1), 0.3f, 0.6f);
    chem.organicComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 2), 0.1f, 0.3f);

    return chem;
}

PlanetChemistry PlanetChemistry::frozenWorld(uint32_t seed) {
    PlanetChemistry chem;

    // Ammonia or methane as solvent for cold worlds
    float solventRoll = PlanetSeed::seedToFloat(PlanetSeed::getSubSeed(seed, ChemistryVariation::SOLVENT_OFFSET));
    chem.solventType = (solventRoll < 0.6f) ? SolventType::AMMONIA : SolventType::METHANE;

    // Thin, nitrogen-rich atmosphere
    uint32_t atmSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::ATMOSPHERE_OFFSET);
    chem.atmosphere.oxygen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 0), 0.0f, 0.05f);
    chem.atmosphere.nitrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 1), 0.7f, 0.98f);
    chem.atmosphere.carbonDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 2), 0.0f, 0.02f);
    chem.atmosphere.methane = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 3), 0.01f, 0.15f);
    chem.atmosphere.argon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 4), 0.01f, 0.05f);
    chem.atmosphere.hydrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 5), 0.0f, 0.1f);
    chem.atmosphere.helium = 0.0f;
    chem.atmosphere.sulfurDioxide = 0.0f;
    chem.atmosphere.pressure = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 6), 0.5f, 2.0f);
    chem.atmosphere.normalize();

    // Low mineral availability (locked in ice)
    uint32_t minSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::MINERALS_OFFSET);
    chem.minerals.iron = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 0), 0.2f, 0.4f);
    chem.minerals.silicon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 1), 0.3f, 0.5f);
    chem.minerals.calcium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 2), 0.1f, 0.3f);
    chem.minerals.sulfur = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 3), 0.05f, 0.15f);
    chem.minerals.phosphorus = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 4), 0.1f, 0.25f);
    chem.minerals.copper = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 5), 0.05f, 0.15f);
    chem.minerals.magnesium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 6), 0.1f, 0.3f);
    chem.minerals.zinc = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 7), 0.02f, 0.1f);

    // Cold, low radiation environment
    chem.radiationLevel = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::RADIATION_OFFSET), 0.3f, 0.8f);
    chem.acidity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::ACIDITY_OFFSET), 6.0f, 9.0f);
    chem.salinity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::SALINITY_OFFSET), 0.0f, 0.02f);

    uint32_t tempSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::TEMPERATURE_OFFSET);
    chem.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 0), -120.0f, -40.0f);
    chem.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 1), 20.0f, 60.0f);

    uint32_t rareSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::RARE_ELEMENTS_OFFSET);
    chem.rareEarthAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 0), 0.1f, 0.3f);
    chem.radioactiveAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 1), 0.02f, 0.1f);
    chem.organicComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 2), 0.2f, 0.5f);

    return chem;
}

PlanetChemistry PlanetChemistry::toxicWorld(uint32_t seed) {
    PlanetChemistry chem;

    // Ethanol or sulfuric acid as solvent
    float solventRoll = PlanetSeed::seedToFloat(PlanetSeed::getSubSeed(seed, ChemistryVariation::SOLVENT_OFFSET));
    chem.solventType = (solventRoll < 0.5f) ? SolventType::ETHANOL : SolventType::SULFURIC_ACID;

    // Toxic atmosphere with high methane and sulfur
    uint32_t atmSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::ATMOSPHERE_OFFSET);
    chem.atmosphere.oxygen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 0), 0.02f, 0.12f);
    chem.atmosphere.nitrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 1), 0.3f, 0.6f);
    chem.atmosphere.carbonDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 2), 0.1f, 0.3f);
    chem.atmosphere.methane = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 3), 0.05f, 0.2f);
    chem.atmosphere.argon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 4), 0.02f, 0.08f);
    chem.atmosphere.hydrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 5), 0.0f, 0.05f);
    chem.atmosphere.helium = 0.0f;
    chem.atmosphere.sulfurDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 6), 0.01f, 0.08f);
    chem.atmosphere.pressure = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 7), 1.5f, 8.0f);
    chem.atmosphere.normalize();

    // High sulfur and unusual mineral distribution
    uint32_t minSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::MINERALS_OFFSET);
    chem.minerals.iron = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 0), 0.3f, 0.6f);
    chem.minerals.silicon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 1), 0.4f, 0.7f);
    chem.minerals.calcium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 2), 0.15f, 0.35f);
    chem.minerals.sulfur = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 3), 0.6f, 0.95f);
    chem.minerals.phosphorus = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 4), 0.2f, 0.5f);
    chem.minerals.copper = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 5), 0.2f, 0.5f);
    chem.minerals.magnesium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 6), 0.2f, 0.4f);
    chem.minerals.zinc = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 7), 0.1f, 0.3f);

    // High radiation, extreme acidity
    chem.radiationLevel = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::RADIATION_OFFSET), 1.0f, 1.8f);
    chem.acidity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::ACIDITY_OFFSET), 2.0f, 5.5f);
    chem.salinity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::SALINITY_OFFSET), 0.05f, 0.2f);

    uint32_t tempSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::TEMPERATURE_OFFSET);
    chem.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 0), 30.0f, 80.0f);
    chem.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 1), 40.0f, 100.0f);

    uint32_t rareSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::RARE_ELEMENTS_OFFSET);
    chem.rareEarthAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 0), 0.3f, 0.7f);
    chem.radioactiveAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 1), 0.15f, 0.4f);
    chem.organicComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 2), 0.3f, 0.6f);

    return chem;
}

PlanetChemistry PlanetChemistry::oceanWorld(uint32_t seed) {
    PlanetChemistry chem;

    chem.solventType = SolventType::WATER;

    // Oxygen-rich, humid atmosphere
    uint32_t atmSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::ATMOSPHERE_OFFSET);
    chem.atmosphere.oxygen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 0), 0.20f, 0.28f);
    chem.atmosphere.nitrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 1), 0.68f, 0.78f);
    chem.atmosphere.carbonDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 2), 0.0003f, 0.003f);
    chem.atmosphere.methane = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 3), 0.0f, 0.0005f);
    chem.atmosphere.argon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 4), 0.005f, 0.012f);
    chem.atmosphere.hydrogen = 0.0f;
    chem.atmosphere.helium = 0.0f;
    chem.atmosphere.sulfurDioxide = 0.0f;
    chem.atmosphere.pressure = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 5), 0.9f, 1.4f);
    chem.atmosphere.normalize();

    // High calcium and magnesium (from shells, coral)
    uint32_t minSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::MINERALS_OFFSET);
    chem.minerals.iron = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 0), 0.3f, 0.5f);
    chem.minerals.silicon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 1), 0.4f, 0.6f);
    chem.minerals.calcium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 2), 0.6f, 0.9f);
    chem.minerals.sulfur = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 3), 0.15f, 0.35f);
    chem.minerals.phosphorus = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 4), 0.25f, 0.5f);
    chem.minerals.copper = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 5), 0.15f, 0.35f);
    chem.minerals.magnesium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 6), 0.5f, 0.8f);
    chem.minerals.zinc = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 7), 0.08f, 0.18f);

    // Moderate conditions, higher salinity
    chem.radiationLevel = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::RADIATION_OFFSET), 0.7f, 1.1f);
    chem.acidity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::ACIDITY_OFFSET), 7.5f, 8.5f);
    chem.salinity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::SALINITY_OFFSET), 0.03f, 0.08f);

    uint32_t tempSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::TEMPERATURE_OFFSET);
    chem.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 0), 18.0f, 28.0f);
    chem.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 1), 15.0f, 35.0f);

    uint32_t rareSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::RARE_ELEMENTS_OFFSET);
    chem.rareEarthAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 0), 0.25f, 0.5f);
    chem.radioactiveAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 1), 0.05f, 0.15f);
    chem.organicComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 2), 0.6f, 0.95f);

    return chem;
}

PlanetChemistry PlanetChemistry::desertWorld(uint32_t seed) {
    PlanetChemistry chem;

    chem.solventType = SolventType::WATER;

    // Thin, dry atmosphere with high CO2
    uint32_t atmSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::ATMOSPHERE_OFFSET);
    chem.atmosphere.oxygen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 0), 0.15f, 0.22f);
    chem.atmosphere.nitrogen = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 1), 0.70f, 0.82f);
    chem.atmosphere.carbonDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 2), 0.002f, 0.02f);
    chem.atmosphere.methane = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 3), 0.0f, 0.0003f);
    chem.atmosphere.argon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 4), 0.01f, 0.03f);
    chem.atmosphere.hydrogen = 0.0f;
    chem.atmosphere.helium = 0.0f;
    chem.atmosphere.sulfurDioxide = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 5), 0.0f, 0.001f);
    chem.atmosphere.pressure = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(atmSeed, 6), 0.4f, 0.9f);
    chem.atmosphere.normalize();

    // High silicon and iron, low calcium (no oceans for shells)
    uint32_t minSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::MINERALS_OFFSET);
    chem.minerals.iron = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 0), 0.5f, 0.85f);
    chem.minerals.silicon = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 1), 0.7f, 0.95f);
    chem.minerals.calcium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 2), 0.1f, 0.3f);
    chem.minerals.sulfur = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 3), 0.2f, 0.4f);
    chem.minerals.phosphorus = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 4), 0.15f, 0.3f);
    chem.minerals.copper = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 5), 0.2f, 0.4f);
    chem.minerals.magnesium = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 6), 0.3f, 0.5f);
    chem.minerals.zinc = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(minSeed, 7), 0.05f, 0.15f);

    // High radiation, neutral to slightly alkaline (dry conditions)
    chem.radiationLevel = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::RADIATION_OFFSET), 1.3f, 1.9f);
    chem.acidity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::ACIDITY_OFFSET), 7.0f, 9.0f);
    chem.salinity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, ChemistryVariation::SALINITY_OFFSET), 0.08f, 0.2f);

    uint32_t tempSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::TEMPERATURE_OFFSET);
    chem.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 0), 25.0f, 50.0f);
    chem.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(tempSeed, 1), 60.0f, 120.0f);

    uint32_t rareSeed = PlanetSeed::getSubSeed(seed, ChemistryVariation::RARE_ELEMENTS_OFFSET);
    chem.rareEarthAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 0), 0.3f, 0.6f);
    chem.radioactiveAbundance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 1), 0.1f, 0.3f);
    chem.organicComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(rareSeed, 2), 0.15f, 0.4f);

    return chem;
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

std::string PlanetChemistry::getProfileName() const {
    std::string name;

    // Solvent type prefix
    switch (solventType) {
        case SolventType::WATER:
            if (atmosphere.oxygen > 0.25f) name = "Oxygen-Rich ";
            else if (atmosphere.oxygen < 0.15f) name = "Low-Oxygen ";
            break;
        case SolventType::AMMONIA:
            name = "Ammonia-Based ";
            break;
        case SolventType::METHANE:
            name = "Cryogenic ";
            break;
        case SolventType::SULFURIC_ACID:
            name = "Sulfuric ";
            break;
        case SolventType::ETHANOL:
            name = "Organic-Solvent ";
            break;
        default:
            break;
    }

    // Temperature classification
    if (temperatureBase < -50.0f) {
        name += "Frozen World";
    } else if (temperatureBase < 0.0f) {
        name += "Cold World";
    } else if (temperatureBase < 35.0f) {
        name += "Temperate World";
    } else if (temperatureBase < 100.0f) {
        name += "Hot World";
    } else if (temperatureBase < 200.0f) {
        name += "Scorched World";
    } else {
        name += "Infernal World";
    }

    // Acidity modifier
    if (acidity < 4.0f) {
        name = "Acidic " + name;
    } else if (acidity > 9.0f) {
        name = "Alkaline " + name;
    }

    return name;
}

bool PlanetChemistry::isCarbonFriendly() const {
    // Check if planet can support carbon-based life
    return solventType == SolventType::WATER &&
           atmosphere.oxygen >= 0.10f &&
           atmosphere.oxygen <= 0.35f &&
           acidity >= 5.0f && acidity <= 9.0f &&
           temperatureBase >= -20.0f && temperatureBase <= 60.0f &&
           minerals.phosphorus >= 0.1f &&
           organicComplexity >= 0.2f;
}

void PlanetChemistry::getLifeTemperatureRange(float& minTemp, float& maxTemp) const {
    // Determine viable temperature range based on solvent type
    switch (solventType) {
        case SolventType::WATER:
            minTemp = -10.0f;   // Slightly below freezing (antifreeze adaptations)
            maxTemp = 120.0f;   // Extremophile limit
            break;
        case SolventType::AMMONIA:
            minTemp = -100.0f;  // Well below ammonia freezing
            maxTemp = -20.0f;   // Above ammonia boiling = death
            break;
        case SolventType::METHANE:
            minTemp = -200.0f;  // Cryogenic life
            maxTemp = -150.0f;
            break;
        case SolventType::SULFURIC_ACID:
            minTemp = 50.0f;
            maxTemp = 350.0f;
            break;
        case SolventType::ETHANOL:
            minTemp = -80.0f;
            maxTemp = 70.0f;
            break;
        default:
            minTemp = 0.0f;
            maxTemp = 100.0f;
    }
}

float PlanetChemistry::getEarthLifeToxicity() const {
    float toxicity = 0.0f;

    // Non-water solvents are lethal
    if (solventType != SolventType::WATER) {
        toxicity += 0.5f;
    }

    // Oxygen too low or too high
    if (atmosphere.oxygen < 0.15f) {
        toxicity += (0.15f - atmosphere.oxygen) * 2.0f;
    } else if (atmosphere.oxygen > 0.30f) {
        toxicity += (atmosphere.oxygen - 0.30f) * 3.0f;
    }

    // High CO2 is toxic
    if (atmosphere.carbonDioxide > 0.01f) {
        toxicity += (atmosphere.carbonDioxide - 0.01f) * 5.0f;
    }

    // High sulfur dioxide is toxic
    toxicity += atmosphere.sulfurDioxide * 10.0f;

    // Extreme acidity/alkalinity
    if (acidity < 5.0f) {
        toxicity += (5.0f - acidity) * 0.1f;
    } else if (acidity > 9.0f) {
        toxicity += (acidity - 9.0f) * 0.1f;
    }

    // Extreme temperatures
    if (temperatureBase < -30.0f) {
        toxicity += (-30.0f - temperatureBase) * 0.01f;
    } else if (temperatureBase > 50.0f) {
        toxicity += (temperatureBase - 50.0f) * 0.01f;
    }

    // High radiation
    if (radiationLevel > 1.5f) {
        toxicity += (radiationLevel - 1.5f) * 0.5f;
    }

    // Extreme pressure
    if (atmosphere.pressure < 0.5f) {
        toxicity += (0.5f - atmosphere.pressure) * 0.3f;
    } else if (atmosphere.pressure > 3.0f) {
        toxicity += (atmosphere.pressure - 3.0f) * 0.1f;
    }

    return std::min(toxicity, 1.0f);
}
