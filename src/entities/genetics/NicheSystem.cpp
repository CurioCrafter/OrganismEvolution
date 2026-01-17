/**
 * @file NicheSystem.cpp
 * @brief Implementation of the ecological niche system
 *
 * Implements comprehensive niche modeling including:
 * - Niche assignment based on genome traits
 * - Pianka's niche overlap index calculations
 * - Competition pressure and fitness evaluation
 * - Niche partitioning and character displacement detection
 */

#include "NicheSystem.h"
#include "../Creature.h"
#include "../../environment/Terrain.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <fstream>
#include <iostream>

namespace genetics {

// =============================================================================
// NICHE CHARACTERISTICS IMPLEMENTATION
// =============================================================================

NicheCharacteristics::NicheCharacteristics() {
    // Initialize with default preferences
    resourcePreferences[ResourceType::PLANT_MATTER] = 0.5f;
    habitatSuitability[HabitatType::PLAINS] = 0.8f;
    habitatSuitability[HabitatType::FOREST] = 0.5f;
}

NicheCharacteristics::NicheCharacteristics(NicheType type) : NicheCharacteristics() {
    // Configure based on niche type
    switch (type) {
        case NicheType::GRAZER:
            primaryResource = ResourceType::PLANT_MATTER;
            resourcePreferences[ResourceType::PLANT_MATTER] = 1.0f;
            huntingStrategy = HuntingStrategy::FORAGING;
            primaryHabitat = HabitatType::PLAINS;
            nicheWidth = 0.4f;
            break;

        case NicheType::BROWSER:
            primaryResource = ResourceType::PLANT_MATTER;
            resourcePreferences[ResourceType::PLANT_MATTER] = 0.8f;
            resourcePreferences[ResourceType::FRUIT] = 0.3f;
            huntingStrategy = HuntingStrategy::FORAGING;
            primaryHabitat = HabitatType::FOREST;
            nicheWidth = 0.5f;
            break;

        case NicheType::FRUGIVORE:
            primaryResource = ResourceType::FRUIT;
            resourcePreferences[ResourceType::FRUIT] = 1.0f;
            resourcePreferences[ResourceType::SEEDS] = 0.4f;
            huntingStrategy = HuntingStrategy::FORAGING;
            primaryHabitat = HabitatType::FOREST;
            nicheWidth = 0.3f;
            break;

        case NicheType::AMBUSH_PREDATOR:
            primaryResource = ResourceType::LIVE_PREY;
            resourcePreferences[ResourceType::LIVE_PREY] = 1.0f;
            huntingStrategy = HuntingStrategy::AMBUSH;
            huntingEfficiency = 0.7f;
            primaryHabitat = HabitatType::FOREST;
            nicheWidth = 0.4f;
            territoriality = 0.8f;
            break;

        case NicheType::PURSUIT_PREDATOR:
            primaryResource = ResourceType::LIVE_PREY;
            resourcePreferences[ResourceType::LIVE_PREY] = 1.0f;
            huntingStrategy = HuntingStrategy::PURSUIT;
            huntingEfficiency = 0.6f;
            foragingRange = 40.0f;
            primaryHabitat = HabitatType::PLAINS;
            nicheWidth = 0.5f;
            territoriality = 0.6f;
            break;

        case NicheType::SCAVENGER:
            primaryResource = ResourceType::CARRION;
            resourcePreferences[ResourceType::CARRION] = 1.0f;
            resourcePreferences[ResourceType::DETRITUS] = 0.3f;
            huntingStrategy = HuntingStrategy::SCAVENGING;
            foragingRange = 50.0f;
            nicheWidth = 0.7f;
            break;

        case NicheType::FILTER_FEEDER:
            primaryResource = ResourceType::PLANKTON;
            resourcePreferences[ResourceType::PLANKTON] = 1.0f;
            huntingStrategy = HuntingStrategy::FILTER;
            primaryHabitat = HabitatType::FRESHWATER;
            secondaryHabitat = HabitatType::MARINE;
            nicheWidth = 0.3f;
            break;

        case NicheType::PARASITE:
            primaryResource = ResourceType::HOST_TISSUE;
            resourcePreferences[ResourceType::HOST_TISSUE] = 1.0f;
            huntingStrategy = HuntingStrategy::PARASITIC;
            nicheWidth = 0.2f;
            break;

        case NicheType::SYMBIONT:
            primaryResource = ResourceType::DETRITUS;
            resourcePreferences[ResourceType::DETRITUS] = 0.5f;
            nicheWidth = 0.4f;
            competitiveAbility = 0.3f;
            break;

        case NicheType::POLLINATOR:
            primaryResource = ResourceType::NECTAR;
            resourcePreferences[ResourceType::NECTAR] = 1.0f;
            huntingStrategy = HuntingStrategy::FORAGING;
            nicheWidth = 0.4f;
            break;

        case NicheType::SEED_DISPERSER:
            primaryResource = ResourceType::FRUIT;
            secondaryResource = ResourceType::SEEDS;
            resourcePreferences[ResourceType::FRUIT] = 0.8f;
            resourcePreferences[ResourceType::SEEDS] = 0.6f;
            huntingStrategy = HuntingStrategy::FORAGING;
            nicheWidth = 0.5f;
            break;

        default:
            break;
    }
}

float NicheCharacteristics::distanceTo(const NicheCharacteristics& other) const {
    float distance = 0.0f;
    int dimensions = 0;

    // Resource preference distance
    for (int r = 0; r < static_cast<int>(ResourceType::COUNT); r++) {
        ResourceType type = static_cast<ResourceType>(r);
        float pref1 = 0.0f, pref2 = 0.0f;

        auto it1 = resourcePreferences.find(type);
        auto it2 = other.resourcePreferences.find(type);

        if (it1 != resourcePreferences.end()) pref1 = it1->second;
        if (it2 != other.resourcePreferences.end()) pref2 = it2->second;

        distance += (pref1 - pref2) * (pref1 - pref2);
        dimensions++;
    }

    // Habitat preference distance
    for (int h = 0; h < static_cast<int>(HabitatType::COUNT); h++) {
        HabitatType type = static_cast<HabitatType>(h);
        float suit1 = 0.0f, suit2 = 0.0f;

        auto it1 = habitatSuitability.find(type);
        auto it2 = other.habitatSuitability.find(type);

        if (it1 != habitatSuitability.end()) suit1 = it1->second;
        if (it2 != other.habitatSuitability.end()) suit2 = it2->second;

        distance += (suit1 - suit2) * (suit1 - suit2);
        dimensions++;
    }

    // Activity pattern distance
    float activityDiff = std::abs(peakActivityTime - other.peakActivityTime) / 24.0f;
    distance += activityDiff * activityDiff;
    dimensions++;

    // Niche width distance
    float widthDiff = nicheWidth - other.nicheWidth;
    distance += widthDiff * widthDiff;
    dimensions++;

    return dimensions > 0 ? std::sqrt(distance / dimensions) : 0.0f;
}

float NicheCharacteristics::calculateOverlap(const NicheCharacteristics& other) const {
    // Pianka's overlap index: sum(p1i * p2i) / sqrt(sum(p1i^2) * sum(p2i^2))
    float sumProduct = 0.0f;
    float sumSq1 = 0.0f;
    float sumSq2 = 0.0f;

    // Resource overlap
    for (int r = 0; r < static_cast<int>(ResourceType::COUNT); r++) {
        ResourceType type = static_cast<ResourceType>(r);
        float p1 = 0.0f, p2 = 0.0f;

        auto it1 = resourcePreferences.find(type);
        auto it2 = other.resourcePreferences.find(type);

        if (it1 != resourcePreferences.end()) p1 = it1->second;
        if (it2 != other.resourcePreferences.end()) p2 = it2->second;

        sumProduct += p1 * p2;
        sumSq1 += p1 * p1;
        sumSq2 += p2 * p2;
    }

    float denominator = std::sqrt(sumSq1 * sumSq2);
    if (denominator < 0.0001f) return 0.0f;

    return sumProduct / denominator;
}

float NicheCharacteristics::evaluateEnvironmentSuitability(
    float temperature, float moisture, float elevation) const {

    float suitability = 1.0f;

    // Temperature tolerance
    if (temperature < temperatureRange.x) {
        float diff = temperatureRange.x - temperature;
        suitability *= std::max(0.0f, 1.0f - diff * 2.0f);
    } else if (temperature > temperatureRange.y) {
        float diff = temperature - temperatureRange.y;
        suitability *= std::max(0.0f, 1.0f - diff * 2.0f);
    }

    // Moisture tolerance
    if (moisture < moistureRange.x) {
        float diff = moistureRange.x - moisture;
        suitability *= std::max(0.0f, 1.0f - diff * 2.0f);
    } else if (moisture > moistureRange.y) {
        float diff = moisture - moistureRange.y;
        suitability *= std::max(0.0f, 1.0f - diff * 2.0f);
    }

    // Elevation tolerance
    if (elevation < elevationRange.x) {
        float diff = (elevationRange.x - elevation) / 100.0f;
        suitability *= std::max(0.0f, 1.0f - diff);
    } else if (elevation > elevationRange.y) {
        float diff = (elevation - elevationRange.y) / 100.0f;
        suitability *= std::max(0.0f, 1.0f - diff);
    }

    return std::clamp(suitability, 0.0f, 1.0f);
}

// =============================================================================
// NICHE COMPETITION IMPLEMENTATION
// =============================================================================

NicheCompetition::NicheCompetition(NicheType n1, NicheType n2)
    : niche1(n1), niche2(n2) {
}

void NicheCompetition::update(float newOverlap, int generation) {
    totalOverlap = newOverlap;
    generationsPersisted++;

    if (!isActive && newOverlap > 0.1f) {
        isActive = true;
        firstDetectedGeneration = generation;
    }

    overlapHistory.push_back(newOverlap);
    while (overlapHistory.size() > MAX_HISTORY) {
        overlapHistory.pop_front();
    }

    // Calculate fitness impacts based on overlap and densities
    float densityFactor = std::min(density1, density2) / 100.0f;
    fitnessImpact1 = newOverlap * competitionCoefficient21 * densityFactor;
    fitnessImpact2 = newOverlap * competitionCoefficient12 * densityFactor;
}

float NicheCompetition::calculateTrend() const {
    if (overlapHistory.size() < 10) {
        return 0.0f;
    }

    // Simple linear regression slope
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;
    int n = static_cast<int>(overlapHistory.size());

    int i = 0;
    for (float overlap : overlapHistory) {
        sumX += static_cast<float>(i);
        sumY += overlap;
        sumXY += static_cast<float>(i) * overlap;
        sumXX += static_cast<float>(i * i);
        i++;
    }

    float denominator = n * sumXX - sumX * sumX;
    if (std::abs(denominator) < 0.0001f) return 0.0f;

    return (n * sumXY - sumX * sumY) / denominator;
}

int NicheCompetition::predictOutcome() const {
    if (!isActive) return 0;

    // Compare competitive abilities
    float advantage1 = competitionCoefficient21 * density1;
    float advantage2 = competitionCoefficient12 * density2;

    // Check for stable coexistence conditions
    bool coexistence = (competitionCoefficient12 < 1.0f && competitionCoefficient21 < 1.0f);

    if (coexistence) return 0;  // Coexistence predicted

    float diff = advantage1 - advantage2;
    if (std::abs(diff) < 0.1f) return 0;  // Too close to call

    return diff > 0 ? 1 : -1;
}

// =============================================================================
// NICHE MANAGER IMPLEMENTATION
// =============================================================================

NicheManager::NicheManager() {
    initialize();
}

NicheManager::NicheManager(const NicheConfig& config)
    : m_config(config) {
    initialize();
}

void NicheManager::initialize() {
    if (m_initialized) return;

    initializeNicheCharacteristics();

    // Initialize occupancy tracking for all niches
    for (int n = 0; n < static_cast<int>(NicheType::COUNT); n++) {
        NicheType type = static_cast<NicheType>(n);
        if (type != NicheType::COUNT && type != NicheType::UNDEFINED) {
            m_nicheOccupancy[type] = NicheOccupancy();
            m_nicheOccupancy[type].nicheType = type;
        }
    }

    m_initialized = true;
}

void NicheManager::reset() {
    m_creatureNiches.clear();
    m_competitions.clear();
    m_nicheShifts.clear();
    m_partitionEvents.clear();
    m_displacementEvents.clear();
    m_previousTraits.clear();

    for (auto& [type, occupancy] : m_nicheOccupancy) {
        occupancy = NicheOccupancy();
        occupancy.nicheType = type;
    }

    m_lastUpdateGeneration = 0;
}

void NicheManager::initializeNicheCharacteristics() {
    m_nicheCharacteristics[NicheType::GRAZER] = createGrazerCharacteristics();
    m_nicheCharacteristics[NicheType::BROWSER] = createBrowserCharacteristics();
    m_nicheCharacteristics[NicheType::FRUGIVORE] = createFrugivoreCharacteristics();
    m_nicheCharacteristics[NicheType::AMBUSH_PREDATOR] = createAmbushPredatorCharacteristics();
    m_nicheCharacteristics[NicheType::PURSUIT_PREDATOR] = createPursuitPredatorCharacteristics();
    m_nicheCharacteristics[NicheType::SCAVENGER] = createScavengerCharacteristics();
    m_nicheCharacteristics[NicheType::FILTER_FEEDER] = createFilterFeederCharacteristics();
    m_nicheCharacteristics[NicheType::PARASITE] = createParasiteCharacteristics();
    m_nicheCharacteristics[NicheType::SYMBIONT] = createSymbiontCharacteristics();
    m_nicheCharacteristics[NicheType::POLLINATOR] = createPollinatorCharacteristics();
    m_nicheCharacteristics[NicheType::SEED_DISPERSER] = createSeedDisperserCharacteristics();
}

// -----------------------------------------------------------------------------
// Niche characteristic creation helpers
// -----------------------------------------------------------------------------

NicheCharacteristics NicheManager::createGrazerCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::PLANT_MATTER;
    chars.resourcePreferences[ResourceType::PLANT_MATTER] = 1.0f;
    chars.huntingStrategy = HuntingStrategy::FORAGING;
    chars.huntingEfficiency = 0.8f;
    chars.foragingRange = 15.0f;
    chars.activityPattern = ActivityPattern::DIURNAL;
    chars.peakActivityTime = 10.0f;
    chars.primaryHabitat = HabitatType::PLAINS;
    chars.secondaryHabitat = HabitatType::WETLAND;
    chars.habitatSuitability[HabitatType::PLAINS] = 1.0f;
    chars.habitatSuitability[HabitatType::WETLAND] = 0.7f;
    chars.habitatSuitability[HabitatType::FOREST] = 0.3f;
    chars.minVegetationDensity = 0.3f;
    chars.maxVegetationDensity = 0.8f;
    chars.nicheWidth = 0.4f;
    chars.dietBreadth = 0.3f;
    chars.territoriality = 0.2f;
    chars.competitiveAbility = 0.4f;
    chars.intraspecificTolerance = 0.8f;
    return chars;
}

NicheCharacteristics NicheManager::createBrowserCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::PLANT_MATTER;
    chars.secondaryResource = ResourceType::FRUIT;
    chars.resourcePreferences[ResourceType::PLANT_MATTER] = 0.8f;
    chars.resourcePreferences[ResourceType::FRUIT] = 0.4f;
    chars.huntingStrategy = HuntingStrategy::FORAGING;
    chars.huntingEfficiency = 0.7f;
    chars.foragingRange = 20.0f;
    chars.activityPattern = ActivityPattern::CREPUSCULAR;
    chars.peakActivityTime = 7.0f;
    chars.primaryHabitat = HabitatType::FOREST;
    chars.secondaryHabitat = HabitatType::ECOTONE;
    chars.habitatSuitability[HabitatType::FOREST] = 1.0f;
    chars.habitatSuitability[HabitatType::ECOTONE] = 0.8f;
    chars.habitatSuitability[HabitatType::PLAINS] = 0.4f;
    chars.minVegetationDensity = 0.5f;
    chars.maxVegetationDensity = 1.0f;
    chars.nicheWidth = 0.5f;
    chars.dietBreadth = 0.5f;
    chars.territoriality = 0.4f;
    chars.competitiveAbility = 0.5f;
    return chars;
}

NicheCharacteristics NicheManager::createFrugivoreCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::FRUIT;
    chars.secondaryResource = ResourceType::SEEDS;
    chars.resourcePreferences[ResourceType::FRUIT] = 1.0f;
    chars.resourcePreferences[ResourceType::SEEDS] = 0.5f;
    chars.resourcePreferences[ResourceType::NECTAR] = 0.2f;
    chars.huntingStrategy = HuntingStrategy::FORAGING;
    chars.huntingEfficiency = 0.6f;
    chars.foragingRange = 25.0f;
    chars.activityPattern = ActivityPattern::DIURNAL;
    chars.peakActivityTime = 11.0f;
    chars.primaryHabitat = HabitatType::FOREST;
    chars.habitatSuitability[HabitatType::FOREST] = 1.0f;
    chars.habitatSuitability[HabitatType::WETLAND] = 0.5f;
    chars.minVegetationDensity = 0.6f;
    chars.nicheWidth = 0.3f;
    chars.dietBreadth = 0.4f;
    chars.territoriality = 0.3f;
    return chars;
}

NicheCharacteristics NicheManager::createAmbushPredatorCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::LIVE_PREY;
    chars.resourcePreferences[ResourceType::LIVE_PREY] = 1.0f;
    chars.resourcePreferences[ResourceType::CARRION] = 0.2f;
    chars.huntingStrategy = HuntingStrategy::AMBUSH;
    chars.huntingEfficiency = 0.7f;
    chars.preySizePreference = 0.0f;  // Flexible prey size
    chars.foragingRange = 15.0f;
    chars.activityPattern = ActivityPattern::CREPUSCULAR;
    chars.peakActivityTime = 6.0f;
    chars.primaryHabitat = HabitatType::FOREST;
    chars.secondaryHabitat = HabitatType::WETLAND;
    chars.habitatSuitability[HabitatType::FOREST] = 1.0f;
    chars.habitatSuitability[HabitatType::WETLAND] = 0.7f;
    chars.habitatSuitability[HabitatType::CAVE] = 0.6f;
    chars.minVegetationDensity = 0.4f;
    chars.nicheWidth = 0.4f;
    chars.dietBreadth = 0.3f;
    chars.territoriality = 0.9f;
    chars.competitiveAbility = 0.7f;
    chars.intraspecificTolerance = 0.3f;
    return chars;
}

NicheCharacteristics NicheManager::createPursuitPredatorCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::LIVE_PREY;
    chars.resourcePreferences[ResourceType::LIVE_PREY] = 1.0f;
    chars.huntingStrategy = HuntingStrategy::PURSUIT;
    chars.huntingEfficiency = 0.5f;  // Lower per-hunt but more attempts
    chars.preySizePreference = -0.3f;  // Prefer smaller, faster prey
    chars.foragingRange = 50.0f;
    chars.activityPattern = ActivityPattern::DIURNAL;
    chars.peakActivityTime = 9.0f;
    chars.primaryHabitat = HabitatType::PLAINS;
    chars.secondaryHabitat = HabitatType::DESERT;
    chars.habitatSuitability[HabitatType::PLAINS] = 1.0f;
    chars.habitatSuitability[HabitatType::DESERT] = 0.6f;
    chars.habitatSuitability[HabitatType::ECOTONE] = 0.7f;
    chars.maxVegetationDensity = 0.5f;
    chars.nicheWidth = 0.5f;
    chars.dietBreadth = 0.4f;
    chars.territoriality = 0.7f;
    chars.competitiveAbility = 0.8f;
    return chars;
}

NicheCharacteristics NicheManager::createScavengerCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::CARRION;
    chars.secondaryResource = ResourceType::DETRITUS;
    chars.resourcePreferences[ResourceType::CARRION] = 1.0f;
    chars.resourcePreferences[ResourceType::DETRITUS] = 0.4f;
    chars.resourcePreferences[ResourceType::INSECTS] = 0.3f;
    chars.huntingStrategy = HuntingStrategy::SCAVENGING;
    chars.huntingEfficiency = 0.8f;
    chars.foragingRange = 60.0f;
    chars.activityPattern = ActivityPattern::CATHEMERAL;
    chars.peakActivityTime = 12.0f;
    chars.primaryHabitat = HabitatType::PLAINS;
    chars.habitatSuitability[HabitatType::PLAINS] = 0.9f;
    chars.habitatSuitability[HabitatType::DESERT] = 0.8f;
    chars.habitatSuitability[HabitatType::FOREST] = 0.6f;
    chars.habitatSuitability[HabitatType::WETLAND] = 0.5f;
    chars.nicheWidth = 0.8f;  // Very generalist
    chars.dietBreadth = 0.7f;
    chars.habitatBreadth = 0.8f;
    chars.territoriality = 0.2f;
    chars.competitiveAbility = 0.4f;
    chars.intraspecificTolerance = 0.7f;
    return chars;
}

NicheCharacteristics NicheManager::createFilterFeederCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::PLANKTON;
    chars.resourcePreferences[ResourceType::PLANKTON] = 1.0f;
    chars.resourcePreferences[ResourceType::DETRITUS] = 0.3f;
    chars.huntingStrategy = HuntingStrategy::FILTER;
    chars.huntingEfficiency = 0.9f;
    chars.foragingRange = 5.0f;
    chars.activityPattern = ActivityPattern::CATHEMERAL;
    chars.primaryHabitat = HabitatType::FRESHWATER;
    chars.secondaryHabitat = HabitatType::MARINE;
    chars.habitatSuitability[HabitatType::FRESHWATER] = 1.0f;
    chars.habitatSuitability[HabitatType::MARINE] = 0.8f;
    chars.habitatSuitability[HabitatType::WETLAND] = 0.6f;
    chars.nicheWidth = 0.3f;  // Specialist
    chars.dietBreadth = 0.2f;
    chars.territoriality = 0.1f;
    chars.intraspecificTolerance = 0.9f;
    return chars;
}

NicheCharacteristics NicheManager::createParasiteCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::HOST_TISSUE;
    chars.resourcePreferences[ResourceType::HOST_TISSUE] = 1.0f;
    chars.huntingStrategy = HuntingStrategy::PARASITIC;
    chars.huntingEfficiency = 0.95f;
    chars.foragingRange = 2.0f;
    chars.activityPattern = ActivityPattern::CATHEMERAL;
    chars.nicheWidth = 0.1f;  // Extreme specialist
    chars.dietBreadth = 0.1f;
    chars.habitatBreadth = 0.2f;
    chars.territoriality = 0.0f;
    chars.competitiveAbility = 0.2f;
    return chars;
}

NicheCharacteristics NicheManager::createSymbiontCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::DETRITUS;
    chars.secondaryResource = ResourceType::NECTAR;
    chars.resourcePreferences[ResourceType::DETRITUS] = 0.6f;
    chars.resourcePreferences[ResourceType::NECTAR] = 0.4f;
    chars.huntingStrategy = HuntingStrategy::FORAGING;
    chars.huntingEfficiency = 0.7f;
    chars.foragingRange = 10.0f;
    chars.activityPattern = ActivityPattern::DIURNAL;
    chars.nicheWidth = 0.4f;
    chars.territoriality = 0.1f;
    chars.competitiveAbility = 0.3f;
    chars.intraspecificTolerance = 0.9f;
    return chars;
}

NicheCharacteristics NicheManager::createPollinatorCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::NECTAR;
    chars.resourcePreferences[ResourceType::NECTAR] = 1.0f;
    chars.resourcePreferences[ResourceType::FRUIT] = 0.2f;
    chars.huntingStrategy = HuntingStrategy::FORAGING;
    chars.huntingEfficiency = 0.85f;
    chars.foragingRange = 30.0f;
    chars.activityPattern = ActivityPattern::DIURNAL;
    chars.peakActivityTime = 11.0f;
    chars.primaryHabitat = HabitatType::PLAINS;
    chars.secondaryHabitat = HabitatType::FOREST;
    chars.habitatSuitability[HabitatType::PLAINS] = 0.9f;
    chars.habitatSuitability[HabitatType::FOREST] = 0.8f;
    chars.habitatSuitability[HabitatType::WETLAND] = 0.6f;
    chars.minVegetationDensity = 0.3f;
    chars.nicheWidth = 0.5f;
    chars.dietBreadth = 0.3f;
    chars.territoriality = 0.3f;
    chars.intraspecificTolerance = 0.7f;
    return chars;
}

NicheCharacteristics NicheManager::createSeedDisperserCharacteristics() const {
    NicheCharacteristics chars;
    chars.primaryResource = ResourceType::FRUIT;
    chars.secondaryResource = ResourceType::SEEDS;
    chars.resourcePreferences[ResourceType::FRUIT] = 0.9f;
    chars.resourcePreferences[ResourceType::SEEDS] = 0.7f;
    chars.resourcePreferences[ResourceType::INSECTS] = 0.3f;
    chars.huntingStrategy = HuntingStrategy::FORAGING;
    chars.huntingEfficiency = 0.7f;
    chars.foragingRange = 40.0f;
    chars.activityPattern = ActivityPattern::DIURNAL;
    chars.peakActivityTime = 10.0f;
    chars.primaryHabitat = HabitatType::FOREST;
    chars.habitatSuitability[HabitatType::FOREST] = 1.0f;
    chars.habitatSuitability[HabitatType::ECOTONE] = 0.7f;
    chars.minVegetationDensity = 0.4f;
    chars.nicheWidth = 0.6f;
    chars.dietBreadth = 0.5f;
    chars.habitatBreadth = 0.5f;
    chars.territoriality = 0.2f;
    return chars;
}

// =============================================================================
// NICHE ASSIGNMENT
// =============================================================================

NicheType NicheManager::assignNiche(Creature* creature) {
    if (!creature) return NicheType::UNDEFINED;

    NicheType assignedNiche = assignNicheFromGenome(creature->getDiploidGenome());

    // Store the assignment
    m_creatureNiches[creature->getID()] = assignedNiche;

    return assignedNiche;
}

NicheType NicheManager::assignNicheFromGenome(const DiploidGenome& genome) const {
    Phenotype phenotype = genome.express();

    // Extract key traits for niche determination
    float aggression = phenotype.aggression;
    float speed = phenotype.speed / 20.0f;  // Normalize to 0-1 range roughly
    float size = phenotype.size;
    float dietSpec = phenotype.dietSpecialization;
    float habitatPref = phenotype.habitatPreference;
    float activityTime = phenotype.activityTime;
    float aquaticAptitude = phenotype.aquaticAptitude;
    float camouflage = phenotype.camouflageLevel;
    float smellSensitivity = phenotype.smellSensitivity;
    float sociality = phenotype.sociality;

    // Calculate niche scores
    std::map<NicheType, float> scores;

    // Pursuit Predator: High aggression + high speed
    scores[NicheType::PURSUIT_PREDATOR] = aggression * 0.4f + speed * 0.4f +
                                          (1.0f - sociality) * 0.1f + size * 0.1f;

    // Ambush Predator: High aggression + high camouflage/stealth
    scores[NicheType::AMBUSH_PREDATOR] = aggression * 0.35f + camouflage * 0.35f +
                                         (1.0f - speed) * 0.15f + size * 0.15f;

    // Grazer: Low aggression + moderate size + high sociality
    scores[NicheType::GRAZER] = (1.0f - aggression) * 0.3f + sociality * 0.3f +
                                (1.0f - dietSpec) * 0.2f + (1.0f - camouflage) * 0.2f;

    // Browser: Low aggression + larger size + forest habitat preference
    scores[NicheType::BROWSER] = (1.0f - aggression) * 0.25f + size * 0.25f +
                                 habitatPref * 0.25f + (1.0f - speed) * 0.25f;

    // Frugivore: Low aggression + diet specialization + small-medium size
    scores[NicheType::FRUGIVORE] = (1.0f - aggression) * 0.2f + dietSpec * 0.4f +
                                   (1.0f - size) * 0.2f + habitatPref * 0.2f;

    // Scavenger: Low-medium aggression + high smell + generalist
    scores[NicheType::SCAVENGER] = smellSensitivity * 0.4f + (1.0f - dietSpec) * 0.3f +
                                   (0.5f - std::abs(aggression - 0.3f)) * 0.3f;

    // Filter Feeder: High aquatic aptitude + low aggression
    scores[NicheType::FILTER_FEEDER] = aquaticAptitude * 0.6f + (1.0f - aggression) * 0.3f +
                                       (1.0f - speed) * 0.1f;

    // Pollinator: Small size + high activity + low aggression
    scores[NicheType::POLLINATOR] = (1.0f - size) * 0.3f + speed * 0.2f +
                                    (1.0f - aggression) * 0.3f + activityTime * 0.2f;

    // Seed Disperser: Medium size + moderate speed + forest preference
    scores[NicheType::SEED_DISPERSER] = (0.5f - std::abs(size - 0.5f)) * 0.3f +
                                        habitatPref * 0.3f + (1.0f - aggression) * 0.2f +
                                        speed * 0.2f;

    // Parasite: Very small + specialized
    scores[NicheType::PARASITE] = (1.0f - size) * 0.5f + dietSpec * 0.3f +
                                  (1.0f - sociality) * 0.2f;

    // Symbiont: Medium traits, cooperative
    scores[NicheType::SYMBIONT] = sociality * 0.4f + (1.0f - aggression) * 0.3f +
                                  (0.5f - std::abs(size - 0.5f)) * 0.3f;

    // Apply stochastic element for edge cases
    for (auto& [type, score] : scores) {
        score += Random::value() * 0.05f;
    }

    // Find highest scoring niche
    NicheType bestNiche = NicheType::GRAZER;  // Default
    float bestScore = 0.0f;

    for (const auto& [type, score] : scores) {
        if (score > bestScore) {
            bestScore = score;
            bestNiche = type;
        }
    }

    return bestNiche;
}

NicheType NicheManager::getNiche(const Creature* creature) const {
    if (!creature) return NicheType::UNDEFINED;

    auto it = m_creatureNiches.find(creature->getID());
    return (it != m_creatureNiches.end()) ? it->second : NicheType::UNDEFINED;
}

std::map<NicheType, float> NicheManager::calculateNicheFit(const Creature* creature) const {
    std::map<NicheType, float> fits;

    if (!creature) return fits;

    const DiploidGenome& genome = creature->getDiploidGenome();
    NicheCharacteristics creatureChars = calculateCreatureCharacteristics(creature);

    for (const auto& [type, nicheChars] : m_nicheCharacteristics) {
        float overlap = creatureChars.calculateOverlap(nicheChars);
        float distance = creatureChars.distanceTo(nicheChars);
        fits[type] = overlap * (1.0f - distance);
    }

    return fits;
}

// =============================================================================
// NICHE OVERLAP AND COMPETITION
// =============================================================================

float NicheManager::calculateNicheOverlap(NicheType niche1, NicheType niche2) const {
    auto it1 = m_nicheCharacteristics.find(niche1);
    auto it2 = m_nicheCharacteristics.find(niche2);

    if (it1 == m_nicheCharacteristics.end() || it2 == m_nicheCharacteristics.end()) {
        return 0.0f;
    }

    return calculateCharacteristicsOverlap(it1->second, it2->second);
}

float NicheManager::calculateCharacteristicsOverlap(
    const NicheCharacteristics& chars1,
    const NicheCharacteristics& chars2) const {

    // Calculate three components of overlap and combine them
    float resourceOverlap = calculateResourceOverlap(chars1, chars2);
    float habitatOverlap = calculateHabitatOverlap(chars1, chars2);
    float temporalOverlap = calculateTemporalOverlap(chars1, chars2);

    // Multiplicative combination - overlap in all dimensions required
    return resourceOverlap * habitatOverlap * temporalOverlap;
}

float NicheManager::calculateResourceOverlap(
    const NicheCharacteristics& chars1,
    const NicheCharacteristics& chars2) const {

    // Pianka's overlap index
    float sumProduct = 0.0f;
    float sumSq1 = 0.0f;
    float sumSq2 = 0.0f;

    for (int r = 0; r < static_cast<int>(ResourceType::COUNT); r++) {
        ResourceType type = static_cast<ResourceType>(r);
        float p1 = 0.0f, p2 = 0.0f;

        auto it1 = chars1.resourcePreferences.find(type);
        auto it2 = chars2.resourcePreferences.find(type);

        if (it1 != chars1.resourcePreferences.end()) p1 = it1->second;
        if (it2 != chars2.resourcePreferences.end()) p2 = it2->second;

        sumProduct += p1 * p2;
        sumSq1 += p1 * p1;
        sumSq2 += p2 * p2;
    }

    float denominator = std::sqrt(sumSq1 * sumSq2);
    if (denominator < 0.0001f) return 0.0f;

    return sumProduct / denominator;
}

float NicheManager::calculateHabitatOverlap(
    const NicheCharacteristics& chars1,
    const NicheCharacteristics& chars2) const {

    float sumProduct = 0.0f;
    float sumSq1 = 0.0f;
    float sumSq2 = 0.0f;

    for (int h = 0; h < static_cast<int>(HabitatType::COUNT); h++) {
        HabitatType type = static_cast<HabitatType>(h);
        float p1 = 0.0f, p2 = 0.0f;

        auto it1 = chars1.habitatSuitability.find(type);
        auto it2 = chars2.habitatSuitability.find(type);

        if (it1 != chars1.habitatSuitability.end()) p1 = it1->second;
        if (it2 != chars2.habitatSuitability.end()) p2 = it2->second;

        sumProduct += p1 * p2;
        sumSq1 += p1 * p1;
        sumSq2 += p2 * p2;
    }

    float denominator = std::sqrt(sumSq1 * sumSq2);
    if (denominator < 0.0001f) return 0.0f;

    return sumProduct / denominator;
}

float NicheManager::calculateTemporalOverlap(
    const NicheCharacteristics& chars1,
    const NicheCharacteristics& chars2) const {

    // Calculate overlap based on activity patterns
    if (chars1.activityPattern == chars2.activityPattern) {
        return 1.0f;  // Same pattern = full temporal overlap
    }

    // Partial overlaps
    if ((chars1.activityPattern == ActivityPattern::CATHEMERAL) ||
        (chars2.activityPattern == ActivityPattern::CATHEMERAL)) {
        return 0.7f;  // Cathemeral overlaps with everything
    }

    if ((chars1.activityPattern == ActivityPattern::CREPUSCULAR) ||
        (chars2.activityPattern == ActivityPattern::CREPUSCULAR)) {
        return 0.5f;  // Crepuscular partial overlap
    }

    // Diurnal vs Nocturnal = minimal overlap
    if ((chars1.activityPattern == ActivityPattern::DIURNAL &&
         chars2.activityPattern == ActivityPattern::NOCTURNAL) ||
        (chars1.activityPattern == ActivityPattern::NOCTURNAL &&
         chars2.activityPattern == ActivityPattern::DIURNAL)) {
        return 0.1f;
    }

    return 0.5f;  // Default moderate overlap
}

const NicheCompetition* NicheManager::getCompetition(NicheType niche1, NicheType niche2) const {
    auto key = makeNichePair(niche1, niche2);
    auto it = m_competitions.find(key);
    return (it != m_competitions.end()) ? &it->second : nullptr;
}

void NicheManager::updateCompetition(const std::vector<Creature*>& creatures, int generation) {
    m_lastUpdateGeneration = generation;

    // Update occupancy first
    trackNicheOccupancy(creatures, generation);

    // Update trait tracking for displacement detection
    updateTraitTracking(creatures);

    // Calculate competition between all niche pairs with occupants
    for (int n1 = 0; n1 < static_cast<int>(NicheType::COUNT) - 1; n1++) {
        NicheType type1 = static_cast<NicheType>(n1);
        if (type1 == NicheType::UNDEFINED) continue;

        auto occ1It = m_nicheOccupancy.find(type1);
        if (occ1It == m_nicheOccupancy.end() || occ1It->second.isEmpty()) continue;

        for (int n2 = n1 + 1; n2 < static_cast<int>(NicheType::COUNT); n2++) {
            NicheType type2 = static_cast<NicheType>(n2);
            if (type2 == NicheType::UNDEFINED || type2 == NicheType::COUNT) continue;

            auto occ2It = m_nicheOccupancy.find(type2);
            if (occ2It == m_nicheOccupancy.end() || occ2It->second.isEmpty()) continue;

            // Calculate overlap
            float overlap = calculateNicheOverlap(type1, type2);

            if (overlap > m_config.minSignificantOverlap) {
                auto key = makeNichePair(type1, type2);
                auto& competition = m_competitions[key];

                if (competition.niche1 == NicheType::UNDEFINED) {
                    competition = NicheCompetition(type1, type2);
                }

                competition.density1 = static_cast<float>(occ1It->second.currentPopulation);
                competition.density2 = static_cast<float>(occ2It->second.currentPopulation);

                // Calculate competition coefficients
                auto chars1It = m_nicheCharacteristics.find(type1);
                auto chars2It = m_nicheCharacteristics.find(type2);

                if (chars1It != m_nicheCharacteristics.end() &&
                    chars2It != m_nicheCharacteristics.end()) {
                    competition.competitionCoefficient12 =
                        overlap * chars2It->second.competitiveAbility;
                    competition.competitionCoefficient21 =
                        overlap * chars1It->second.competitiveAbility;
                }

                competition.resourceOverlap = calculateResourceOverlap(
                    m_nicheCharacteristics.at(type1),
                    m_nicheCharacteristics.at(type2));
                competition.habitatOverlap = calculateHabitatOverlap(
                    m_nicheCharacteristics.at(type1),
                    m_nicheCharacteristics.at(type2));
                competition.temporalOverlap = calculateTemporalOverlap(
                    m_nicheCharacteristics.at(type1),
                    m_nicheCharacteristics.at(type2));

                competition.update(overlap, generation);
            }
        }
    }
}

std::vector<NicheCompetition> NicheManager::getActiveCompetitions() const {
    std::vector<NicheCompetition> active;
    for (const auto& [key, competition] : m_competitions) {
        if (competition.isActive) {
            active.push_back(competition);
        }
    }
    return active;
}

// =============================================================================
// EMPTY NICHE DETECTION
// =============================================================================

std::vector<EmptyNicheInfo> NicheManager::detectEmptyNiches(const Terrain& terrain) const {
    std::vector<EmptyNicheInfo> emptyNiches;

    for (int n = 0; n < static_cast<int>(NicheType::COUNT); n++) {
        NicheType type = static_cast<NicheType>(n);
        if (type == NicheType::UNDEFINED || type == NicheType::COUNT) continue;

        auto occIt = m_nicheOccupancy.find(type);
        if (occIt != m_nicheOccupancy.end() && !occIt->second.isEmpty()) continue;

        // This niche is empty - evaluate if resources exist
        auto charIt = m_nicheCharacteristics.find(type);
        if (charIt == m_nicheCharacteristics.end()) continue;

        EmptyNicheInfo info;
        info.nicheType = type;
        info.regionCenter = glm::vec3(
            terrain.getWidth() * terrain.getScale() * 0.5f,
            0.0f,
            terrain.getDepth() * terrain.getScale() * 0.5f);
        info.regionRadius = std::min(terrain.getWidth(), terrain.getDepth()) *
                            terrain.getScale() * 0.5f;

        // Estimate resource availability based on niche type and terrain
        // This is a simplified estimation
        const auto& chars = charIt->second;
        float resourceScore = 0.0f;

        if (chars.primaryHabitat == HabitatType::FRESHWATER ||
            chars.primaryHabitat == HabitatType::MARINE) {
            // Check for water presence
            int waterCount = 0;
            int sampleCount = 0;
            for (int x = 0; x < terrain.getWidth(); x += 10) {
                for (int z = 0; z < terrain.getDepth(); z += 10) {
                    if (terrain.isWater(x * terrain.getScale(), z * terrain.getScale())) {
                        waterCount++;
                    }
                    sampleCount++;
                }
            }
            resourceScore = sampleCount > 0 ? static_cast<float>(waterCount) / sampleCount : 0.0f;
        } else {
            // Land-based resources depend on vegetation (approximated by terrain height variation)
            resourceScore = 0.5f + Random::value() * 0.3f;  // Placeholder
        }

        info.resourceAvailability = resourceScore;

        // Find nearest source niche
        float nearestDistance = std::numeric_limits<float>::max();
        for (const auto& [otherType, otherChars] : m_nicheCharacteristics) {
            if (otherType == type) continue;

            auto otherOccIt = m_nicheOccupancy.find(otherType);
            if (otherOccIt == m_nicheOccupancy.end() || otherOccIt->second.isEmpty()) continue;

            float distance = chars.distanceTo(otherChars);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                info.nearestSourceNiche = otherType;
            }
        }

        // Fitness potential based on resources and lack of competition
        info.fitnessPotential = info.resourceAvailability *
                                (1.0f + m_config.emptyNicheBonus);

        info.reason = "No species currently occupying this niche";

        if (info.resourceAvailability > 0.2f) {
            emptyNiches.push_back(info);
        }
    }

    return emptyNiches;
}

std::vector<EmptyNicheInfo> NicheManager::detectEmptyNiches(
    const std::vector<Creature*>& creatures) const {

    std::vector<EmptyNicheInfo> emptyNiches;

    // Check each niche type
    for (int n = 0; n < static_cast<int>(NicheType::COUNT); n++) {
        NicheType type = static_cast<NicheType>(n);
        if (type == NicheType::UNDEFINED || type == NicheType::COUNT) continue;

        // Check if any creature occupies this niche
        bool hasOccupants = false;
        for (const Creature* c : creatures) {
            if (c && c->isAlive()) {
                auto it = m_creatureNiches.find(c->getID());
                if (it != m_creatureNiches.end() && it->second == type) {
                    hasOccupants = true;
                    break;
                }
            }
        }

        if (!hasOccupants) {
            EmptyNicheInfo info;
            info.nicheType = type;

            // Find average position of all creatures as reference
            glm::vec3 avgPos(0.0f);
            int count = 0;
            for (const Creature* c : creatures) {
                if (c && c->isAlive()) {
                    avgPos += c->getPosition();
                    count++;
                }
            }
            if (count > 0) avgPos /= static_cast<float>(count);

            info.regionCenter = avgPos;
            info.regionRadius = 100.0f;

            // Estimate based on niche characteristics
            auto charIt = m_nicheCharacteristics.find(type);
            if (charIt != m_nicheCharacteristics.end()) {
                info.resourceAvailability = 0.5f;  // Assume moderate
                info.fitnessPotential = 0.5f * (1.0f + m_config.emptyNicheBonus);
            }

            info.reason = "No creatures assigned to this niche";
            emptyNiches.push_back(info);
        }
    }

    return emptyNiches;
}

bool NicheManager::isNicheEmpty(NicheType nicheType, const glm::vec3& center,
                                float radius) const {
    // Check occupancy data
    auto it = m_nicheOccupancy.find(nicheType);
    if (it == m_nicheOccupancy.end()) return true;

    return it->second.isEmpty();
}

// =============================================================================
// NICHE WIDTH CALCULATION
// =============================================================================

float NicheManager::calculateNicheWidth(const Creature* creature) const {
    if (!creature) return 0.5f;
    return calculateNicheWidthFromGenome(creature->getDiploidGenome());
}

float NicheManager::calculateNicheWidthFromGenome(const DiploidGenome& genome) const {
    Phenotype p = genome.express();

    // Diet breadth: how specialized is the diet?
    // Low diet specialization = generalist = wider niche
    float dietBreadth = 1.0f - p.dietSpecialization;

    // Habitat breadth: tolerance ranges
    float habitatBreadth = (p.heatTolerance + p.coldTolerance) / 2.0f;

    // Activity flexibility
    float activityBreadth = 0.5f;  // Base value, could be refined

    // Combine with weights
    float nicheWidth = dietBreadth * 0.4f + habitatBreadth * 0.35f + activityBreadth * 0.25f;

    return std::clamp(nicheWidth, 0.0f, 1.0f);
}

bool NicheManager::isSpecialist(const Creature* creature) const {
    return calculateNicheWidth(creature) < 0.4f;
}

bool NicheManager::isGeneralist(const Creature* creature) const {
    return calculateNicheWidth(creature) > 0.6f;
}

// =============================================================================
// NICHE OCCUPANCY TRACKING
// =============================================================================

void NicheManager::trackNicheOccupancy(const std::vector<Creature*>& creatures,
                                       int generation) {
    // Reset counts
    for (auto& [type, occupancy] : m_nicheOccupancy) {
        occupancy.currentPopulation = 0;
        occupancy.speciesCount = 0;
        occupancy.occupyingSpecies.clear();
    }

    std::map<NicheType, std::set<SpeciesId>> speciesPerNiche;
    std::map<NicheType, float> fitnessSum;
    std::map<NicheType, float> widthSum;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        NicheType niche = getNiche(c);
        if (niche == NicheType::UNDEFINED) {
            niche = assignNiche(c);
        }

        auto& occupancy = m_nicheOccupancy[niche];
        occupancy.currentPopulation++;
        fitnessSum[niche] += c->getFitness();
        widthSum[niche] += calculateNicheWidth(c);
        speciesPerNiche[niche].insert(c->getDiploidGenome().getSpeciesId());
    }

    // Calculate averages and update
    for (auto& [type, occupancy] : m_nicheOccupancy) {
        if (occupancy.currentPopulation > 0) {
            occupancy.averageFitness = fitnessSum[type] / occupancy.currentPopulation;
            occupancy.averageNicheWidth = widthSum[type] / occupancy.currentPopulation;

            for (SpeciesId sid : speciesPerNiche[type]) {
                occupancy.occupyingSpecies.push_back(sid);
            }
            occupancy.speciesCount = static_cast<int>(occupancy.occupyingSpecies.size());
        }

        occupancy.update(occupancy.currentPopulation, generation);
    }
}

const NicheOccupancy& NicheManager::getOccupancy(NicheType nicheType) const {
    static NicheOccupancy empty;
    auto it = m_nicheOccupancy.find(nicheType);
    return (it != m_nicheOccupancy.end()) ? it->second : empty;
}

const std::map<NicheType, NicheOccupancy>& NicheManager::getAllOccupancy() const {
    return m_nicheOccupancy;
}

std::vector<Creature*> NicheManager::getCreaturesInNiche(
    NicheType nicheType,
    const std::vector<Creature*>& creatures) const {

    std::vector<Creature*> result;
    for (Creature* c : creatures) {
        if (c && c->isAlive() && getNiche(c) == nicheType) {
            result.push_back(c);
        }
    }
    return result;
}

// =============================================================================
// COMPETITION PRESSURE
// =============================================================================

float NicheManager::calculateCompetitionPressure(const Creature* creature) const {
    if (!creature) return 0.0f;

    NicheType niche = getNiche(creature);
    if (niche == NicheType::UNDEFINED) return 0.0f;

    float pressure = 0.0f;

    // Intraspecific competition (within niche)
    auto occIt = m_nicheOccupancy.find(niche);
    if (occIt != m_nicheOccupancy.end()) {
        const auto& occupancy = occIt->second;

        // Pressure increases with population relative to carrying capacity
        float densityPressure = static_cast<float>(occupancy.currentPopulation) /
                               std::max(1, occupancy.estimatedCarryingCapacity);
        densityPressure = std::min(densityPressure, 2.0f);  // Cap at 2x carrying capacity

        pressure += densityPressure * 0.5f;

        // Additional pressure if multiple species in same niche
        if (occupancy.speciesCount > 1) {
            pressure += (occupancy.speciesCount - 1) * 0.1f;
        }
    }

    // Interspecific competition from overlapping niches
    for (const auto& [key, competition] : m_competitions) {
        if (competition.niche1 == niche || competition.niche2 == niche) {
            float overlap = competition.totalOverlap;
            float otherDensity = (competition.niche1 == niche) ?
                                 competition.density2 : competition.density1;

            pressure += overlap * (otherDensity / 100.0f) * 0.3f;
        }
    }

    // Apply niche width modifier - generalists feel less pressure
    float nicheWidth = calculateNicheWidth(creature);
    pressure *= (1.0f - nicheWidth * 0.3f);

    return std::clamp(pressure, 0.0f, 1.0f);
}

float NicheManager::calculateIntraspecificCompetition(
    const Creature* creature,
    const std::vector<Creature*>& conspecifics) const {

    if (!creature || conspecifics.empty()) return 0.0f;

    float totalCompetition = 0.0f;
    glm::vec3 myPos = creature->getPosition();
    NicheCharacteristics myChars = calculateCreatureCharacteristics(creature);
    float myForagingRange = myChars.foragingRange;

    for (const Creature* other : conspecifics) {
        if (!other || other == creature || !other->isAlive()) continue;

        float distance = glm::length(other->getPosition() - myPos);

        // Competition decreases with distance
        if (distance < myForagingRange * 2.0f) {
            float distanceFactor = 1.0f - (distance / (myForagingRange * 2.0f));
            totalCompetition += distanceFactor * (1.0f - myChars.intraspecificTolerance);
        }
    }

    // Normalize by expected number of competitors
    float expected = static_cast<float>(conspecifics.size()) * 0.5f;
    if (expected > 0) {
        totalCompetition = totalCompetition / expected;
    }

    return std::clamp(totalCompetition, 0.0f, 1.0f);
}

float NicheManager::calculateInterspecificCompetition(
    const Creature* creature,
    const std::vector<Creature*>& heterospecifics) const {

    if (!creature || heterospecifics.empty()) return 0.0f;

    float totalCompetition = 0.0f;
    NicheType myNiche = getNiche(creature);
    NicheCharacteristics myChars = calculateCreatureCharacteristics(creature);

    for (const Creature* other : heterospecifics) {
        if (!other || !other->isAlive()) continue;

        NicheType otherNiche = getNiche(other);
        if (otherNiche == NicheType::UNDEFINED) continue;

        // Get niche overlap
        float overlap = calculateNicheOverlap(myNiche, otherNiche);
        if (overlap < 0.1f) continue;

        // Distance factor
        float distance = glm::length(other->getPosition() - creature->getPosition());
        float distanceFactor = 1.0f;
        if (distance > myChars.foragingRange) {
            distanceFactor = std::max(0.0f, 1.0f - (distance - myChars.foragingRange) /
                                                   myChars.foragingRange);
        }

        totalCompetition += overlap * distanceFactor;
    }

    return std::clamp(totalCompetition / 10.0f, 0.0f, 1.0f);  // Normalize
}

// =============================================================================
// NICHE FITNESS EVALUATION
// =============================================================================

float NicheManager::evaluateNicheFitness(const Creature* creature) const {
    if (!creature) return 1.0f;

    NicheType niche = getNiche(creature);
    if (niche == NicheType::UNDEFINED) return 0.8f;

    float fitness = 1.0f;

    // How well does creature fit its assigned niche?
    NicheCharacteristics creatureChars = calculateCreatureCharacteristics(creature);
    auto nicheCharsIt = m_nicheCharacteristics.find(niche);

    if (nicheCharsIt != m_nicheCharacteristics.end()) {
        float overlap = creatureChars.calculateOverlap(nicheCharsIt->second);
        float distance = creatureChars.distanceTo(nicheCharsIt->second);

        // Trait match component
        float traitMatch = overlap * (1.0f - distance * 0.5f);
        fitness *= (0.5f + traitMatch * 0.5f);
    }

    // Specialist vs generalist modifier
    fitness *= (1.0f + calculateSpecializationModifier(creature));

    // Competition pressure penalty
    float pressure = calculateCompetitionPressure(creature);
    fitness *= (1.0f - pressure * m_config.competitionPenalty);

    // Empty niche bonus (if this creature is colonizing an underutilized niche)
    auto occIt = m_nicheOccupancy.find(niche);
    if (occIt != m_nicheOccupancy.end()) {
        int pop = occIt->second.currentPopulation;
        int capacity = occIt->second.estimatedCarryingCapacity;
        if (pop < capacity / 4) {
            fitness *= (1.0f + m_config.emptyNicheBonus * (1.0f - static_cast<float>(pop) / (capacity / 4)));
        }
    }

    return std::clamp(fitness, 0.1f, 2.0f);
}

float NicheManager::evaluatePotentialFitness(const Creature* creature,
                                              NicheType targetNiche) const {
    if (!creature) return 0.0f;

    NicheCharacteristics creatureChars = calculateCreatureCharacteristics(creature);
    auto nicheCharsIt = m_nicheCharacteristics.find(targetNiche);

    if (nicheCharsIt == m_nicheCharacteristics.end()) return 0.5f;

    float overlap = creatureChars.calculateOverlap(nicheCharsIt->second);
    float distance = creatureChars.distanceTo(nicheCharsIt->second);

    float potential = overlap * (1.0f - distance);

    // Factor in occupancy of target niche
    auto occIt = m_nicheOccupancy.find(targetNiche);
    if (occIt != m_nicheOccupancy.end()) {
        if (occIt->second.isEmpty()) {
            potential *= (1.0f + m_config.emptyNicheBonus);
        } else {
            float crowding = static_cast<float>(occIt->second.currentPopulation) /
                            std::max(1, occIt->second.estimatedCarryingCapacity);
            potential *= (1.0f - crowding * 0.3f);
        }
    }

    return std::clamp(potential, 0.0f, 1.5f);
}

float NicheManager::calculateSpecializationModifier(const Creature* creature) const {
    if (!creature) return 0.0f;

    float nicheWidth = calculateNicheWidth(creature);

    if (nicheWidth < 0.4f) {
        // Specialist - bonus in optimal conditions
        // Check if creature is in suitable environment
        NicheType niche = getNiche(creature);
        auto occIt = m_nicheOccupancy.find(niche);

        if (occIt != m_nicheOccupancy.end() && !occIt->second.isEmpty()) {
            // Specialist in occupied niche = bonus
            return m_config.specialistBonus * (0.4f - nicheWidth) / 0.4f;
        }
        return 0.0f;  // No bonus if niche empty
    } else if (nicheWidth > 0.6f) {
        // Generalist - smaller but more consistent bonus
        return m_config.generalistBonus * (nicheWidth - 0.6f) / 0.4f;
    }

    return 0.0f;  // Middle ground, no modifier
}

// =============================================================================
// NICHE EVOLUTION TRACKING
// =============================================================================

std::vector<NichePartition> NicheManager::detectNichePartitioning(
    const std::vector<Creature*>& creatures,
    int generation) {

    std::vector<NichePartition> newPartitions;

    if (!m_config.enablePartitioning) return newPartitions;

    // Group creatures by niche and species
    std::map<NicheType, std::map<SpeciesId, std::vector<Creature*>>> nicheSpecies;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        NicheType niche = getNiche(c);
        SpeciesId species = c->getDiploidGenome().getSpeciesId();

        nicheSpecies[niche][species].push_back(c);
    }

    // Look for divergence within niches
    for (const auto& [niche, speciesMap] : nicheSpecies) {
        if (speciesMap.size() < 2) continue;  // Need multiple species

        std::vector<std::pair<SpeciesId, std::vector<Creature*>>> speciesVec(
            speciesMap.begin(), speciesMap.end());

        for (size_t i = 0; i < speciesVec.size(); i++) {
            for (size_t j = i + 1; j < speciesVec.size(); j++) {
                // Compare diet specialization between species
                float diet1 = 0.0f, diet2 = 0.0f;

                for (Creature* c : speciesVec[i].second) {
                    diet1 += c->getDiploidGenome().getTrait(GeneType::DIET_SPECIALIZATION);
                }
                diet1 /= speciesVec[i].second.size();

                for (Creature* c : speciesVec[j].second) {
                    diet2 += c->getDiploidGenome().getTrait(GeneType::DIET_SPECIALIZATION);
                }
                diet2 /= speciesVec[j].second.size();

                float dietDiff = std::abs(diet1 - diet2);

                // Check if difference suggests partitioning
                if (dietDiff > 0.3f) {
                    NichePartition partition;
                    partition.originalNiche = niche;
                    partition.speciesA = speciesVec[i].first;
                    partition.speciesB = speciesVec[j].first;
                    partition.partitionDimension = "diet";
                    partition.generation = generation;
                    partition.positionA = diet1;
                    partition.positionB = diet2;
                    partition.separation = dietDiff;
                    partition.competitionDriven = true;

                    m_partitionEvents.push_back(partition);
                    newPartitions.push_back(partition);
                }
            }
        }
    }

    return newPartitions;
}

std::vector<CharacterDisplacement> NicheManager::detectCharacterDisplacement(
    const std::vector<Creature*>& creatures,
    int generation) {

    std::vector<CharacterDisplacement> newDisplacements;

    if (!m_config.enableDisplacement) return newDisplacements;

    // Group by species
    std::map<SpeciesId, std::vector<Creature*>> speciesCreatures;
    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            speciesCreatures[c->getDiploidGenome().getSpeciesId()].push_back(c);
        }
    }

    // Calculate current trait averages per species
    std::map<SpeciesId, std::map<std::string, float>> currentTraits;

    for (const auto& [species, members] : speciesCreatures) {
        if (members.empty()) continue;

        float sizeSum = 0.0f, dietSum = 0.0f, activitySum = 0.0f;

        for (Creature* c : members) {
            Phenotype p = c->getDiploidGenome().express();
            sizeSum += p.size;
            dietSum += p.dietSpecialization;
            activitySum += p.activityTime;
        }

        float n = static_cast<float>(members.size());
        currentTraits[species]["size"] = sizeSum / n;
        currentTraits[species]["diet"] = dietSum / n;
        currentTraits[species]["activity"] = activitySum / n;
    }

    // Compare to previous generation
    std::vector<SpeciesId> speciesList;
    for (const auto& [species, traits] : currentTraits) {
        speciesList.push_back(species);
    }

    for (size_t i = 0; i < speciesList.size(); i++) {
        for (size_t j = i + 1; j < speciesList.size(); j++) {
            SpeciesId sp1 = speciesList[i];
            SpeciesId sp2 = speciesList[j];

            // Check if both species have previous trait data
            auto prev1It = m_previousTraits.find(sp1);
            auto prev2It = m_previousTraits.find(sp2);

            if (prev1It == m_previousTraits.end() || prev2It == m_previousTraits.end()) {
                continue;
            }

            // Check each trait for displacement
            for (const auto& [trait, currentVal1] : currentTraits[sp1]) {
                float currentVal2 = currentTraits[sp2][trait];

                auto prevTrait1It = prev1It->second.find(trait);
                auto prevTrait2It = prev2It->second.find(trait);

                if (prevTrait1It == prev1It->second.end() ||
                    prevTrait2It == prev2It->second.end()) {
                    continue;
                }

                float prevDiff = std::abs(prevTrait1It->second - prevTrait2It->second);
                float currentDiff = std::abs(currentVal1 - currentVal2);

                // Displacement: species becoming more different
                if (currentDiff - prevDiff > m_config.displacementThreshold) {
                    CharacterDisplacement displacement;
                    displacement.species1 = sp1;
                    displacement.species2 = sp2;
                    displacement.traitName = trait;
                    displacement.startGeneration = generation - 1;
                    displacement.duration = 1;
                    displacement.initialDifference = prevDiff;
                    displacement.finalDifference = currentDiff;
                    displacement.displacementMagnitude = currentDiff - prevDiff;
                    displacement.direction = 1;  // Divergence
                    displacement.ongoing = true;

                    m_displacementEvents.push_back(displacement);
                    newDisplacements.push_back(displacement);
                }
            }
        }
    }

    // Update previous traits for next comparison
    m_previousTraits = currentTraits;

    return newDisplacements;
}

void NicheManager::recordNicheShift(SpeciesId speciesId, NicheType fromNiche,
                                    NicheType toNiche, int generation,
                                    const std::string& cause) {
    NicheShift shift;
    shift.speciesId = speciesId;
    shift.fromNiche = fromNiche;
    shift.toNiche = toNiche;
    shift.generation = generation;
    shift.cause = cause;

    // Check if target niche was empty
    auto toOccIt = m_nicheOccupancy.find(toNiche);
    shift.colonizedEmptyNiche = (toOccIt == m_nicheOccupancy.end() || toOccIt->second.isEmpty());

    m_nicheShifts.push_back(shift);
}

std::vector<NicheShift> NicheManager::getNicheShifts(int sinceGeneration) const {
    if (sinceGeneration <= 0) {
        return m_nicheShifts;
    }

    std::vector<NicheShift> filtered;
    for (const auto& shift : m_nicheShifts) {
        if (shift.generation >= sinceGeneration) {
            filtered.push_back(shift);
        }
    }
    return filtered;
}

const std::vector<NichePartition>& NicheManager::getPartitionEvents() const {
    return m_partitionEvents;
}

const std::vector<CharacterDisplacement>& NicheManager::getDisplacementEvents() const {
    return m_displacementEvents;
}

// =============================================================================
// NICHE CHARACTERISTICS ACCESS
// =============================================================================

const NicheCharacteristics& NicheManager::getNicheCharacteristics(NicheType nicheType) const {
    static NicheCharacteristics defaultChars;
    auto it = m_nicheCharacteristics.find(nicheType);
    return (it != m_nicheCharacteristics.end()) ? it->second : defaultChars;
}

void NicheManager::setNicheCharacteristics(NicheType nicheType,
                                           const NicheCharacteristics& characteristics) {
    m_nicheCharacteristics[nicheType] = characteristics;
}

NicheCharacteristics NicheManager::calculateCreatureCharacteristics(
    const Creature* creature) const {

    NicheCharacteristics chars;

    if (!creature) return chars;

    Phenotype p = creature->getDiploidGenome().express();

    // Determine resource preferences from diet and behavior
    if (p.aggression > 0.5f) {
        chars.primaryResource = ResourceType::LIVE_PREY;
        chars.resourcePreferences[ResourceType::LIVE_PREY] = p.aggression;
        chars.resourcePreferences[ResourceType::CARRION] = (1.0f - p.aggression) * 0.3f;
    } else {
        chars.primaryResource = ResourceType::PLANT_MATTER;
        chars.resourcePreferences[ResourceType::PLANT_MATTER] = 1.0f - p.dietSpecialization;
        chars.resourcePreferences[ResourceType::FRUIT] = p.dietSpecialization * 0.8f;
        chars.resourcePreferences[ResourceType::SEEDS] = p.dietSpecialization * 0.4f;
    }

    // Hunting strategy
    if (p.aggression > 0.7f && p.speed > 15.0f) {
        chars.huntingStrategy = HuntingStrategy::PURSUIT;
    } else if (p.aggression > 0.5f && p.camouflageLevel > 0.5f) {
        chars.huntingStrategy = HuntingStrategy::AMBUSH;
    } else if (p.smellSensitivity > 0.7f) {
        chars.huntingStrategy = HuntingStrategy::SCAVENGING;
    } else {
        chars.huntingStrategy = HuntingStrategy::FORAGING;
    }

    chars.huntingEfficiency = (p.speed + p.visionAcuity) / 2.0f * p.aggression;
    chars.foragingRange = p.speed * 2.0f;

    // Activity pattern
    if (p.activityTime > 0.7f) {
        chars.activityPattern = ActivityPattern::DIURNAL;
        chars.peakActivityTime = 12.0f;
    } else if (p.activityTime < 0.3f) {
        chars.activityPattern = ActivityPattern::NOCTURNAL;
        chars.peakActivityTime = 0.0f;
    } else {
        chars.activityPattern = ActivityPattern::CREPUSCULAR;
        chars.peakActivityTime = 6.0f;
    }

    // Habitat preferences from aptitudes
    if (p.aquaticAptitude > 0.6f) {
        chars.primaryHabitat = HabitatType::FRESHWATER;
        chars.habitatSuitability[HabitatType::FRESHWATER] = p.aquaticAptitude;
        chars.habitatSuitability[HabitatType::MARINE] = p.aquaticAptitude * 0.7f;
    } else if (p.habitatPreference > 0.6f) {
        chars.primaryHabitat = HabitatType::FOREST;
        chars.habitatSuitability[HabitatType::FOREST] = p.habitatPreference;
    } else {
        chars.primaryHabitat = HabitatType::PLAINS;
        chars.habitatSuitability[HabitatType::PLAINS] = 1.0f - p.habitatPreference;
    }

    // Niche dimensions
    chars.nicheWidth = calculateNicheWidthFromGenome(creature->getDiploidGenome());
    chars.dietBreadth = 1.0f - p.dietSpecialization;
    chars.habitatBreadth = (p.heatTolerance + p.coldTolerance) / 2.0f;

    // Social traits
    chars.territoriality = p.aggression * (1.0f - p.sociality);
    chars.competitiveAbility = (p.aggression + p.size) / 2.0f;
    chars.intraspecificTolerance = p.sociality;

    // Temperature and elevation ranges
    chars.temperatureRange = glm::vec2(
        0.5f - p.coldTolerance * 0.5f,
        0.5f + p.heatTolerance * 0.5f);

    return chars;
}

// =============================================================================
// CONFIGURATION
// =============================================================================

void NicheManager::setConfig(const NicheConfig& config) {
    m_config = config;
}

// =============================================================================
// STATISTICS AND REPORTING
// =============================================================================

std::string NicheManager::getNicheStatistics() const {
    std::stringstream ss;

    ss << "=== Niche System Statistics ===\n";
    ss << "Occupied niches: " << getOccupiedNicheCount() << "\n";
    ss << "Empty niches: " << getEmptyNicheCount() << "\n";
    ss << "Active competitions: " << getActiveCompetitions().size() << "\n";
    ss << "Total niche shifts: " << m_nicheShifts.size() << "\n";
    ss << "Partition events: " << m_partitionEvents.size() << "\n";
    ss << "Displacement events: " << m_displacementEvents.size() << "\n";
    ss << "\n--- Niche Occupancy ---\n";

    for (const auto& [type, occupancy] : m_nicheOccupancy) {
        if (occupancy.currentPopulation > 0) {
            ss << nicheTypeToString(type) << ": "
               << occupancy.currentPopulation << " creatures, "
               << occupancy.speciesCount << " species, "
               << "avg fitness: " << occupancy.averageFitness << "\n";
        }
    }

    return ss.str();
}

int NicheManager::getOccupiedNicheCount() const {
    int count = 0;
    for (const auto& [type, occupancy] : m_nicheOccupancy) {
        if (!occupancy.isEmpty()) count++;
    }
    return count;
}

int NicheManager::getEmptyNicheCount() const {
    int total = static_cast<int>(NicheType::COUNT) - 2;  // Exclude UNDEFINED and COUNT
    return total - getOccupiedNicheCount();
}

NicheType NicheManager::getMostCrowdedNiche() const {
    NicheType mostCrowded = NicheType::UNDEFINED;
    int maxPop = 0;

    for (const auto& [type, occupancy] : m_nicheOccupancy) {
        if (occupancy.currentPopulation > maxPop) {
            maxPop = occupancy.currentPopulation;
            mostCrowded = type;
        }
    }

    return mostCrowded;
}

void NicheManager::exportToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for CSV export: " << filename << std::endl;
        return;
    }

    // Header
    file << "NicheType,Population,SpeciesCount,AverageFitness,AverageNicheWidth\n";

    // Data
    for (const auto& [type, occupancy] : m_nicheOccupancy) {
        file << nicheTypeToString(type) << ","
             << occupancy.currentPopulation << ","
             << occupancy.speciesCount << ","
             << occupancy.averageFitness << ","
             << occupancy.averageNicheWidth << "\n";
    }

    file.close();
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

std::string NicheManager::nicheTypeToString(NicheType type) {
    switch (type) {
        case NicheType::GRAZER:           return "Grazer";
        case NicheType::BROWSER:          return "Browser";
        case NicheType::FRUGIVORE:        return "Frugivore";
        case NicheType::AMBUSH_PREDATOR:  return "Ambush Predator";
        case NicheType::PURSUIT_PREDATOR: return "Pursuit Predator";
        case NicheType::SCAVENGER:        return "Scavenger";
        case NicheType::FILTER_FEEDER:    return "Filter Feeder";
        case NicheType::PARASITE:         return "Parasite";
        case NicheType::SYMBIONT:         return "Symbiont";
        case NicheType::POLLINATOR:       return "Pollinator";
        case NicheType::SEED_DISPERSER:   return "Seed Disperser";
        case NicheType::UNDEFINED:        return "Undefined";
        case NicheType::COUNT:            return "COUNT";
        default:                          return "Unknown";
    }
}

NicheType NicheManager::stringToNicheType(const std::string& name) {
    if (name == "Grazer")           return NicheType::GRAZER;
    if (name == "Browser")          return NicheType::BROWSER;
    if (name == "Frugivore")        return NicheType::FRUGIVORE;
    if (name == "Ambush Predator")  return NicheType::AMBUSH_PREDATOR;
    if (name == "Pursuit Predator") return NicheType::PURSUIT_PREDATOR;
    if (name == "Scavenger")        return NicheType::SCAVENGER;
    if (name == "Filter Feeder")    return NicheType::FILTER_FEEDER;
    if (name == "Parasite")         return NicheType::PARASITE;
    if (name == "Symbiont")         return NicheType::SYMBIONT;
    if (name == "Pollinator")       return NicheType::POLLINATOR;
    if (name == "Seed Disperser")   return NicheType::SEED_DISPERSER;
    return NicheType::UNDEFINED;
}

std::string NicheManager::resourceTypeToString(ResourceType type) {
    switch (type) {
        case ResourceType::PLANT_MATTER: return "Plant Matter";
        case ResourceType::FRUIT:        return "Fruit";
        case ResourceType::SEEDS:        return "Seeds";
        case ResourceType::NECTAR:       return "Nectar";
        case ResourceType::LIVE_PREY:    return "Live Prey";
        case ResourceType::CARRION:      return "Carrion";
        case ResourceType::DETRITUS:     return "Detritus";
        case ResourceType::PLANKTON:     return "Plankton";
        case ResourceType::HOST_TISSUE:  return "Host Tissue";
        case ResourceType::INSECTS:      return "Insects";
        case ResourceType::COUNT:        return "COUNT";
        default:                         return "Unknown";
    }
}

std::string NicheManager::huntingStrategyToString(HuntingStrategy strategy) {
    switch (strategy) {
        case HuntingStrategy::NONE:       return "None";
        case HuntingStrategy::AMBUSH:     return "Ambush";
        case HuntingStrategy::PURSUIT:    return "Pursuit";
        case HuntingStrategy::PACK_HUNTING: return "Pack Hunting";
        case HuntingStrategy::FILTER:     return "Filter";
        case HuntingStrategy::FORAGING:   return "Foraging";
        case HuntingStrategy::SCAVENGING: return "Scavenging";
        case HuntingStrategy::PARASITIC:  return "Parasitic";
        case HuntingStrategy::COUNT:      return "COUNT";
        default:                          return "Unknown";
    }
}

std::string NicheManager::activityPatternToString(ActivityPattern pattern) {
    switch (pattern) {
        case ActivityPattern::DIURNAL:    return "Diurnal";
        case ActivityPattern::NOCTURNAL:  return "Nocturnal";
        case ActivityPattern::CREPUSCULAR: return "Crepuscular";
        case ActivityPattern::CATHEMERAL: return "Cathemeral";
        case ActivityPattern::COUNT:      return "COUNT";
        default:                          return "Unknown";
    }
}

std::string NicheManager::habitatTypeToString(HabitatType habitat) {
    switch (habitat) {
        case HabitatType::FOREST:     return "Forest";
        case HabitatType::PLAINS:     return "Plains";
        case HabitatType::DESERT:     return "Desert";
        case HabitatType::WETLAND:    return "Wetland";
        case HabitatType::FRESHWATER: return "Freshwater";
        case HabitatType::MARINE:     return "Marine";
        case HabitatType::MOUNTAIN:   return "Mountain";
        case HabitatType::CAVE:       return "Cave";
        case HabitatType::ECOTONE:    return "Ecotone";
        case HabitatType::COUNT:      return "COUNT";
        default:                      return "Unknown";
    }
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

std::pair<NicheType, NicheType> NicheManager::makeNichePair(NicheType n1, NicheType n2) const {
    // Always order pair with smaller value first for consistent map lookups
    if (static_cast<int>(n1) <= static_cast<int>(n2)) {
        return {n1, n2};
    }
    return {n2, n1};
}

std::vector<float> NicheManager::extractDietTraits(const DiploidGenome& genome) const {
    Phenotype p = genome.express();
    return {
        p.dietSpecialization,
        p.aggression,
        p.size
    };
}

std::vector<float> NicheManager::extractBehaviorTraits(const DiploidGenome& genome) const {
    Phenotype p = genome.express();
    return {
        p.aggression,
        p.sociality,
        p.fearResponse,
        p.camouflageLevel
    };
}

void NicheManager::updateTraitTracking(const std::vector<Creature*>& creatures) {
    // This is called by updateCompetition to prepare for displacement detection
    // The actual trait storage happens in detectCharacterDisplacement
    // This method could be used for additional pre-processing if needed
}

} // namespace genetics
