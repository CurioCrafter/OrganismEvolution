#include "BiomeSystem.h"
#include "PlanetTheme.h"
#include "IslandGenerator.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <cstring>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <unordered_map>

// ============================================================================
// Utility Functions
// ============================================================================

const char* biomeToString(BiomeType type) {
    switch (type) {
        case BiomeType::DEEP_OCEAN: return "Deep Ocean";
        case BiomeType::OCEAN: return "Ocean";
        case BiomeType::SHALLOW_WATER: return "Shallow Water";
        case BiomeType::CORAL_REEF: return "Coral Reef";
        case BiomeType::KELP_FOREST: return "Kelp Forest";
        case BiomeType::BEACH_SANDY: return "Sandy Beach";
        case BiomeType::BEACH_ROCKY: return "Rocky Beach";
        case BiomeType::TIDAL_POOL: return "Tidal Pool";
        case BiomeType::MANGROVE: return "Mangrove";
        case BiomeType::SALT_MARSH: return "Salt Marsh";
        case BiomeType::GRASSLAND: return "Grassland";
        case BiomeType::SAVANNA: return "Savanna";
        case BiomeType::TROPICAL_RAINFOREST: return "Tropical Rainforest";
        case BiomeType::TEMPERATE_FOREST: return "Temperate Forest";
        case BiomeType::SWAMP: return "Swamp";
        case BiomeType::WETLAND: return "Wetland";
        case BiomeType::SHRUBLAND: return "Shrubland";
        case BiomeType::BOREAL_FOREST: return "Boreal Forest";
        case BiomeType::ALPINE_MEADOW: return "Alpine Meadow";
        case BiomeType::ROCKY_HIGHLANDS: return "Rocky Highlands";
        case BiomeType::MOUNTAIN_FOREST: return "Mountain Forest";
        case BiomeType::DESERT_HOT: return "Hot Desert";
        case BiomeType::DESERT_COLD: return "Cold Desert";
        case BiomeType::TUNDRA: return "Tundra";
        case BiomeType::GLACIER: return "Glacier";
        case BiomeType::VOLCANIC: return "Volcanic";
        case BiomeType::LAVA_FIELD: return "Lava Field";
        case BiomeType::CRATER_LAKE: return "Crater Lake";
        case BiomeType::CAVE_ENTRANCE: return "Cave Entrance";
        case BiomeType::RIVER_BANK: return "River Bank";
        case BiomeType::LAKE_SHORE: return "Lake Shore";
        default: return "Unknown";
    }
}

BiomeType stringToBiome(const std::string& name) {
    if (name == "Deep Ocean") return BiomeType::DEEP_OCEAN;
    if (name == "Ocean") return BiomeType::OCEAN;
    if (name == "Shallow Water") return BiomeType::SHALLOW_WATER;
    if (name == "Coral Reef") return BiomeType::CORAL_REEF;
    if (name == "Kelp Forest") return BiomeType::KELP_FOREST;
    if (name == "Sandy Beach") return BiomeType::BEACH_SANDY;
    if (name == "Rocky Beach") return BiomeType::BEACH_ROCKY;
    if (name == "Tidal Pool") return BiomeType::TIDAL_POOL;
    if (name == "Mangrove") return BiomeType::MANGROVE;
    if (name == "Salt Marsh") return BiomeType::SALT_MARSH;
    if (name == "Grassland") return BiomeType::GRASSLAND;
    if (name == "Savanna") return BiomeType::SAVANNA;
    if (name == "Tropical Rainforest") return BiomeType::TROPICAL_RAINFOREST;
    if (name == "Temperate Forest") return BiomeType::TEMPERATE_FOREST;
    if (name == "Swamp") return BiomeType::SWAMP;
    if (name == "Wetland") return BiomeType::WETLAND;
    if (name == "Shrubland") return BiomeType::SHRUBLAND;
    if (name == "Boreal Forest") return BiomeType::BOREAL_FOREST;
    if (name == "Alpine Meadow") return BiomeType::ALPINE_MEADOW;
    if (name == "Rocky Highlands") return BiomeType::ROCKY_HIGHLANDS;
    if (name == "Mountain Forest") return BiomeType::MOUNTAIN_FOREST;
    if (name == "Hot Desert") return BiomeType::DESERT_HOT;
    if (name == "Cold Desert") return BiomeType::DESERT_COLD;
    if (name == "Tundra") return BiomeType::TUNDRA;
    if (name == "Glacier") return BiomeType::GLACIER;
    if (name == "Volcanic") return BiomeType::VOLCANIC;
    if (name == "Lava Field") return BiomeType::LAVA_FIELD;
    if (name == "Crater Lake") return BiomeType::CRATER_LAKE;
    if (name == "Cave Entrance") return BiomeType::CAVE_ENTRANCE;
    if (name == "River Bank") return BiomeType::RIVER_BANK;
    if (name == "Lake Shore") return BiomeType::LAKE_SHORE;
    return BiomeType::GRASSLAND; // Default fallback
}

glm::vec3 getDefaultBiomeColor(BiomeType type) {
    switch (type) {
        // Water biomes
        case BiomeType::DEEP_OCEAN: return glm::vec3(0.05f, 0.10f, 0.35f);
        case BiomeType::OCEAN: return glm::vec3(0.10f, 0.20f, 0.50f);
        case BiomeType::SHALLOW_WATER: return glm::vec3(0.20f, 0.40f, 0.60f);
        case BiomeType::CORAL_REEF: return glm::vec3(0.30f, 0.60f, 0.65f);
        case BiomeType::KELP_FOREST: return glm::vec3(0.15f, 0.35f, 0.30f);

        // Coastal biomes
        case BiomeType::BEACH_SANDY: return glm::vec3(0.85f, 0.78f, 0.55f);
        case BiomeType::BEACH_ROCKY: return glm::vec3(0.50f, 0.45f, 0.40f);
        case BiomeType::TIDAL_POOL: return glm::vec3(0.35f, 0.50f, 0.55f);
        case BiomeType::MANGROVE: return glm::vec3(0.25f, 0.40f, 0.25f);
        case BiomeType::SALT_MARSH: return glm::vec3(0.45f, 0.55f, 0.35f);

        // Lowland biomes
        case BiomeType::GRASSLAND: return glm::vec3(0.45f, 0.65f, 0.25f);
        case BiomeType::SAVANNA: return glm::vec3(0.70f, 0.65f, 0.35f);
        case BiomeType::TROPICAL_RAINFOREST: return glm::vec3(0.15f, 0.45f, 0.15f);
        case BiomeType::TEMPERATE_FOREST: return glm::vec3(0.20f, 0.45f, 0.20f);
        case BiomeType::SWAMP: return glm::vec3(0.30f, 0.40f, 0.25f);
        case BiomeType::WETLAND: return glm::vec3(0.35f, 0.50f, 0.30f);

        // Highland biomes
        case BiomeType::SHRUBLAND: return glm::vec3(0.55f, 0.55f, 0.35f);
        case BiomeType::BOREAL_FOREST: return glm::vec3(0.15f, 0.35f, 0.25f);
        case BiomeType::ALPINE_MEADOW: return glm::vec3(0.50f, 0.60f, 0.40f);
        case BiomeType::ROCKY_HIGHLANDS: return glm::vec3(0.55f, 0.50f, 0.45f);
        case BiomeType::MOUNTAIN_FOREST: return glm::vec3(0.20f, 0.40f, 0.25f);

        // Extreme biomes
        case BiomeType::DESERT_HOT: return glm::vec3(0.85f, 0.70f, 0.45f);
        case BiomeType::DESERT_COLD: return glm::vec3(0.70f, 0.65f, 0.55f);
        case BiomeType::TUNDRA: return glm::vec3(0.75f, 0.80f, 0.75f);
        case BiomeType::GLACIER: return glm::vec3(0.85f, 0.90f, 0.95f);
        case BiomeType::VOLCANIC: return glm::vec3(0.30f, 0.25f, 0.25f);
        case BiomeType::LAVA_FIELD: return glm::vec3(0.80f, 0.30f, 0.10f);
        case BiomeType::CRATER_LAKE: return glm::vec3(0.25f, 0.45f, 0.55f);

        // Special biomes
        case BiomeType::CAVE_ENTRANCE: return glm::vec3(0.25f, 0.22f, 0.20f);
        case BiomeType::RIVER_BANK: return glm::vec3(0.40f, 0.50f, 0.35f);
        case BiomeType::LAKE_SHORE: return glm::vec3(0.50f, 0.55f, 0.40f);

        default: return glm::vec3(0.5f, 0.5f, 0.5f);
    }
}

bool isAquaticBiome(BiomeType type) {
    switch (type) {
        case BiomeType::DEEP_OCEAN:
        case BiomeType::OCEAN:
        case BiomeType::SHALLOW_WATER:
        case BiomeType::CORAL_REEF:
        case BiomeType::KELP_FOREST:
        case BiomeType::TIDAL_POOL:
        case BiomeType::CRATER_LAKE:
            return true;
        default:
            return false;
    }
}

bool isTransitionBiome(BiomeType type) {
    switch (type) {
        case BiomeType::BEACH_SANDY:
        case BiomeType::BEACH_ROCKY:
        case BiomeType::TIDAL_POOL:
        case BiomeType::MANGROVE:
        case BiomeType::SALT_MARSH:
        case BiomeType::RIVER_BANK:
        case BiomeType::LAKE_SHORE:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// BiomeSystem Implementation
// ============================================================================

BiomeSystem::BiomeSystem() {
    initializeBiomeProperties();

    // Initialize default cell
    m_defaultCell.primaryBiome = BiomeType::OCEAN;
    m_defaultCell.secondaryBiome = BiomeType::OCEAN;
    m_defaultCell.blendFactor = 0.0f;
    m_defaultCell.temperature = 0.5f;
    m_defaultCell.moisture = 0.5f;
    m_defaultCell.elevation = 0.0f;
    m_defaultCell.slope = 0.0f;
    m_defaultCell.color = getDefaultBiomeColor(BiomeType::OCEAN);
    m_defaultCell.fertility = 0.0f;
}

BiomeSystem::~BiomeSystem() {
    // Cleanup if needed
}

void BiomeSystem::initializeDefaultBiomes() {
    initializeBiomeProperties();

    // Set up default climate zones (Earth-like)
    m_climateZones.clear();

    // Polar zones
    ClimateZone polarNorth;
    polarNorth.latitudeStart = 0.7f;
    polarNorth.latitudeEnd = 1.0f;
    polarNorth.baseTemperature = -0.8f;
    polarNorth.baseMoisture = 0.3f;
    polarNorth.temperatureVariation = 0.1f;
    polarNorth.moistureVariation = 0.2f;
    m_climateZones.push_back(polarNorth);

    ClimateZone polarSouth;
    polarSouth.latitudeStart = -1.0f;
    polarSouth.latitudeEnd = -0.7f;
    polarSouth.baseTemperature = -0.9f;
    polarSouth.baseMoisture = 0.2f;
    polarSouth.temperatureVariation = 0.1f;
    polarSouth.moistureVariation = 0.15f;
    m_climateZones.push_back(polarSouth);

    // Temperate zones
    ClimateZone temperateNorth;
    temperateNorth.latitudeStart = 0.3f;
    temperateNorth.latitudeEnd = 0.7f;
    temperateNorth.baseTemperature = 0.2f;
    temperateNorth.baseMoisture = 0.6f;
    temperateNorth.temperatureVariation = 0.3f;
    temperateNorth.moistureVariation = 0.3f;
    m_climateZones.push_back(temperateNorth);

    ClimateZone temperateSouth;
    temperateSouth.latitudeStart = -0.7f;
    temperateSouth.latitudeEnd = -0.3f;
    temperateSouth.baseTemperature = 0.15f;
    temperateSouth.baseMoisture = 0.65f;
    temperateSouth.temperatureVariation = 0.25f;
    temperateSouth.moistureVariation = 0.3f;
    m_climateZones.push_back(temperateSouth);

    // Tropical zone
    ClimateZone tropical;
    tropical.latitudeStart = -0.3f;
    tropical.latitudeEnd = 0.3f;
    tropical.baseTemperature = 0.8f;
    tropical.baseMoisture = 0.7f;
    tropical.temperatureVariation = 0.15f;
    tropical.moistureVariation = 0.4f;
    m_climateZones.push_back(tropical);

    // Set up default biome transitions
    m_transitions.clear();
    addTransition(BiomeType::OCEAN, BiomeType::BEACH_SANDY, 0.05f);
    addTransition(BiomeType::BEACH_SANDY, BiomeType::GRASSLAND, 0.08f);
    addTransition(BiomeType::GRASSLAND, BiomeType::TEMPERATE_FOREST, 0.1f);
    addTransition(BiomeType::TEMPERATE_FOREST, BiomeType::MOUNTAIN_FOREST, 0.12f);
    addTransition(BiomeType::GRASSLAND, BiomeType::DESERT_HOT, 0.15f, BiomeType::SAVANNA);
    addTransition(BiomeType::TEMPERATE_FOREST, BiomeType::BOREAL_FOREST, 0.1f);
    addTransition(BiomeType::BOREAL_FOREST, BiomeType::TUNDRA, 0.12f);
    addTransition(BiomeType::TUNDRA, BiomeType::GLACIER, 0.08f);
}

void BiomeSystem::initializeWithTheme(const PlanetTheme& theme) {
    initializeDefaultBiomes();

    // Apply theme colors to biome properties
    const TerrainPalette& terrain = theme.getTerrain();

    // Water biomes
    setBaseColor(BiomeType::DEEP_OCEAN, terrain.deepWaterColor);
    setBaseColor(BiomeType::OCEAN, glm::mix(terrain.deepWaterColor, terrain.shallowWaterColor, 0.3f));
    setBaseColor(BiomeType::SHALLOW_WATER, terrain.shallowWaterColor);
    setBaseColor(BiomeType::CORAL_REEF, glm::mix(terrain.shallowWaterColor, glm::vec3(0.4f, 0.7f, 0.7f), 0.5f));
    setBaseColor(BiomeType::KELP_FOREST, glm::mix(terrain.shallowWaterColor, terrain.forestColor, 0.4f));

    // Coastal biomes
    setBaseColor(BiomeType::BEACH_SANDY, terrain.sandColor);
    setBaseColor(BiomeType::BEACH_ROCKY, terrain.rockColor);
    setBaseColor(BiomeType::MANGROVE, glm::mix(terrain.forestColor, terrain.sandColor, 0.3f));
    setBaseColor(BiomeType::SALT_MARSH, glm::mix(terrain.grassColor, terrain.sandColor, 0.4f));

    // Vegetation biomes
    setBaseColor(BiomeType::GRASSLAND, terrain.grassColor);
    setBaseColor(BiomeType::SAVANNA, glm::mix(terrain.grassColor, terrain.sandColor, 0.4f));
    setBaseColor(BiomeType::TROPICAL_RAINFOREST, terrain.jungleColor);
    setBaseColor(BiomeType::TEMPERATE_FOREST, terrain.forestColor);
    setBaseColor(BiomeType::SWAMP, glm::mix(terrain.forestColor, terrain.dirtColor, 0.3f));
    setBaseColor(BiomeType::WETLAND, glm::mix(terrain.grassColor, terrain.dirtColor, 0.25f));

    // Highland biomes
    setBaseColor(BiomeType::SHRUBLAND, terrain.shrubColor);
    setBaseColor(BiomeType::BOREAL_FOREST, glm::mix(terrain.forestColor, terrain.snowColor, 0.15f));
    setBaseColor(BiomeType::ALPINE_MEADOW, glm::mix(terrain.grassColor, terrain.snowColor, 0.2f));
    setBaseColor(BiomeType::ROCKY_HIGHLANDS, terrain.rockColor);
    setBaseColor(BiomeType::MOUNTAIN_FOREST, glm::mix(terrain.forestColor, terrain.rockColor, 0.2f));

    // Extreme biomes
    setBaseColor(BiomeType::DESERT_HOT, terrain.sandColor);
    setBaseColor(BiomeType::DESERT_COLD, glm::mix(terrain.sandColor, terrain.snowColor, 0.3f));
    setBaseColor(BiomeType::TUNDRA, glm::mix(terrain.snowColor, terrain.rockColor, 0.2f));
    setBaseColor(BiomeType::GLACIER, terrain.glacierColor);
    setBaseColor(BiomeType::VOLCANIC, terrain.ashColor);
    setBaseColor(BiomeType::LAVA_FIELD, terrain.lavaColor);
}

void BiomeSystem::initializeBiomeProperties() {
    m_properties.clear();

    // Helper lambda to create biome properties
    auto createBiome = [this](BiomeType type, const std::string& name,
                               float temp, float moisture, float fertility, float habitability,
                               float minH, float maxH, float minS, float maxS,
                               float treeDen, float grassDen, float shrubDen,
                               float herbCap, float carnCap, float aquaCap, float flyCap) {
        BiomeProperties props;
        props.type = type;
        props.name = name;
        props.baseColor = getDefaultBiomeColor(type);
        props.accentColor = props.baseColor * 0.8f;
        props.roughness = 0.7f;
        props.metallic = 0.0f;
        props.temperature = temp;
        props.moisture = moisture;
        props.fertility = fertility;
        props.habitability = habitability;
        props.minHeight = minH;
        props.maxHeight = maxH;
        props.minSlope = minS;
        props.maxSlope = maxS;
        props.treeDensity = treeDen;
        props.grassDensity = grassDen;
        props.shrubDensity = shrubDen;
        props.herbivoreCapacity = herbCap;
        props.carnivoreCapacity = carnCap;
        props.aquaticCapacity = aquaCap;
        props.flyingCapacity = flyCap;
        m_properties[type] = props;
    };

    // Water biomes (temp, moisture, fertility, habitability, minH, maxH, minS, maxS, tree, grass, shrub, herb, carn, aqua, fly)
    createBiome(BiomeType::DEEP_OCEAN, "Deep Ocean",
                0.1f, 1.0f, 0.1f, 0.3f, 0.0f, 0.15f, 0.0f, 0.1f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.1f);

    createBiome(BiomeType::OCEAN, "Ocean",
                0.2f, 1.0f, 0.2f, 0.5f, 0.15f, 0.25f, 0.0f, 0.1f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.2f);

    createBiome(BiomeType::SHALLOW_WATER, "Shallow Water",
                0.3f, 1.0f, 0.4f, 0.7f, 0.25f, 0.35f, 0.0f, 0.15f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.3f);

    createBiome(BiomeType::CORAL_REEF, "Coral Reef",
                0.6f, 1.0f, 0.8f, 0.9f, 0.20f, 0.32f, 0.0f, 0.2f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.2f);

    createBiome(BiomeType::KELP_FOREST, "Kelp Forest",
                0.3f, 1.0f, 0.7f, 0.85f, 0.18f, 0.30f, 0.0f, 0.15f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.95f, 0.15f);

    // Coastal biomes
    createBiome(BiomeType::BEACH_SANDY, "Sandy Beach",
                0.5f, 0.4f, 0.2f, 0.4f, 0.33f, 0.40f, 0.0f, 0.2f,
                0.0f, 0.1f, 0.1f, 0.2f, 0.1f, 0.3f, 0.4f);

    createBiome(BiomeType::BEACH_ROCKY, "Rocky Beach",
                0.4f, 0.3f, 0.1f, 0.3f, 0.33f, 0.42f, 0.1f, 0.4f,
                0.0f, 0.05f, 0.1f, 0.15f, 0.1f, 0.4f, 0.3f);

    createBiome(BiomeType::TIDAL_POOL, "Tidal Pool",
                0.4f, 0.9f, 0.6f, 0.7f, 0.32f, 0.38f, 0.0f, 0.15f,
                0.0f, 0.0f, 0.0f, 0.1f, 0.05f, 0.8f, 0.2f);

    createBiome(BiomeType::MANGROVE, "Mangrove",
                0.7f, 0.9f, 0.7f, 0.75f, 0.33f, 0.42f, 0.0f, 0.1f,
                0.6f, 0.1f, 0.3f, 0.4f, 0.3f, 0.6f, 0.5f);

    createBiome(BiomeType::SALT_MARSH, "Salt Marsh",
                0.5f, 0.85f, 0.5f, 0.6f, 0.34f, 0.42f, 0.0f, 0.1f,
                0.0f, 0.6f, 0.2f, 0.5f, 0.2f, 0.4f, 0.6f);

    // Lowland biomes
    createBiome(BiomeType::GRASSLAND, "Grassland",
                0.4f, 0.5f, 0.7f, 0.85f, 0.38f, 0.55f, 0.0f, 0.2f,
                0.05f, 0.9f, 0.2f, 0.9f, 0.4f, 0.0f, 0.6f);

    createBiome(BiomeType::SAVANNA, "Savanna",
                0.7f, 0.35f, 0.5f, 0.7f, 0.38f, 0.52f, 0.0f, 0.15f,
                0.15f, 0.7f, 0.3f, 0.8f, 0.5f, 0.0f, 0.5f);

    createBiome(BiomeType::TROPICAL_RAINFOREST, "Tropical Rainforest",
                0.85f, 0.9f, 0.95f, 0.95f, 0.38f, 0.55f, 0.0f, 0.3f,
                0.95f, 0.3f, 0.6f, 0.8f, 0.6f, 0.1f, 0.8f);

    createBiome(BiomeType::TEMPERATE_FOREST, "Temperate Forest",
                0.4f, 0.7f, 0.85f, 0.9f, 0.40f, 0.60f, 0.0f, 0.35f,
                0.85f, 0.4f, 0.5f, 0.85f, 0.5f, 0.05f, 0.7f);

    createBiome(BiomeType::SWAMP, "Swamp",
                0.5f, 0.95f, 0.6f, 0.5f, 0.36f, 0.45f, 0.0f, 0.1f,
                0.4f, 0.2f, 0.4f, 0.4f, 0.3f, 0.7f, 0.5f);

    createBiome(BiomeType::WETLAND, "Wetland",
                0.45f, 0.9f, 0.7f, 0.65f, 0.36f, 0.44f, 0.0f, 0.08f,
                0.1f, 0.5f, 0.3f, 0.6f, 0.25f, 0.5f, 0.7f);

    // Highland biomes
    createBiome(BiomeType::SHRUBLAND, "Shrubland",
                0.5f, 0.4f, 0.5f, 0.6f, 0.50f, 0.65f, 0.1f, 0.4f,
                0.1f, 0.5f, 0.7f, 0.6f, 0.3f, 0.0f, 0.4f);

    createBiome(BiomeType::BOREAL_FOREST, "Boreal Forest",
                -0.2f, 0.6f, 0.6f, 0.7f, 0.45f, 0.65f, 0.0f, 0.35f,
                0.75f, 0.3f, 0.4f, 0.6f, 0.4f, 0.05f, 0.5f);

    createBiome(BiomeType::ALPINE_MEADOW, "Alpine Meadow",
                -0.1f, 0.5f, 0.5f, 0.6f, 0.60f, 0.75f, 0.1f, 0.4f,
                0.0f, 0.7f, 0.4f, 0.5f, 0.2f, 0.0f, 0.6f);

    createBiome(BiomeType::ROCKY_HIGHLANDS, "Rocky Highlands",
                0.0f, 0.3f, 0.2f, 0.3f, 0.65f, 0.80f, 0.3f, 0.7f,
                0.0f, 0.2f, 0.3f, 0.25f, 0.15f, 0.0f, 0.4f);

    createBiome(BiomeType::MOUNTAIN_FOREST, "Mountain Forest",
                0.1f, 0.6f, 0.6f, 0.65f, 0.55f, 0.72f, 0.1f, 0.45f,
                0.7f, 0.3f, 0.5f, 0.55f, 0.35f, 0.0f, 0.55f);

    // Extreme biomes
    createBiome(BiomeType::DESERT_HOT, "Hot Desert",
                0.9f, 0.05f, 0.1f, 0.2f, 0.40f, 0.60f, 0.0f, 0.25f,
                0.0f, 0.05f, 0.1f, 0.15f, 0.1f, 0.0f, 0.2f);

    createBiome(BiomeType::DESERT_COLD, "Cold Desert",
                -0.3f, 0.1f, 0.15f, 0.25f, 0.45f, 0.65f, 0.0f, 0.3f,
                0.0f, 0.1f, 0.15f, 0.2f, 0.1f, 0.0f, 0.15f);

    createBiome(BiomeType::TUNDRA, "Tundra",
                -0.6f, 0.4f, 0.3f, 0.35f, 0.50f, 0.75f, 0.0f, 0.25f,
                0.0f, 0.3f, 0.2f, 0.35f, 0.15f, 0.0f, 0.3f);

    createBiome(BiomeType::GLACIER, "Glacier",
                -0.9f, 0.8f, 0.0f, 0.1f, 0.70f, 1.0f, 0.0f, 0.5f,
                0.0f, 0.0f, 0.0f, 0.05f, 0.02f, 0.0f, 0.1f);

    createBiome(BiomeType::VOLCANIC, "Volcanic",
                0.8f, 0.1f, 0.3f, 0.2f, 0.60f, 0.90f, 0.2f, 0.8f,
                0.0f, 0.1f, 0.1f, 0.1f, 0.05f, 0.0f, 0.2f);

    createBiome(BiomeType::LAVA_FIELD, "Lava Field",
                1.0f, 0.0f, 0.0f, 0.0f, 0.55f, 0.85f, 0.0f, 0.6f,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.05f);

    createBiome(BiomeType::CRATER_LAKE, "Crater Lake",
                0.3f, 1.0f, 0.5f, 0.6f, 0.55f, 0.70f, 0.0f, 0.3f,
                0.0f, 0.0f, 0.0f, 0.1f, 0.05f, 0.7f, 0.3f);

    // Special biomes
    createBiome(BiomeType::CAVE_ENTRANCE, "Cave Entrance",
                0.2f, 0.5f, 0.2f, 0.4f, 0.50f, 0.75f, 0.4f, 0.9f,
                0.0f, 0.1f, 0.2f, 0.3f, 0.2f, 0.1f, 0.4f);

    createBiome(BiomeType::RIVER_BANK, "River Bank",
                0.4f, 0.8f, 0.75f, 0.8f, 0.38f, 0.50f, 0.0f, 0.2f,
                0.3f, 0.6f, 0.4f, 0.7f, 0.35f, 0.5f, 0.6f);

    createBiome(BiomeType::LAKE_SHORE, "Lake Shore",
                0.4f, 0.75f, 0.7f, 0.75f, 0.36f, 0.45f, 0.0f, 0.15f,
                0.2f, 0.6f, 0.3f, 0.65f, 0.3f, 0.4f, 0.55f);
}

void BiomeSystem::generateBiomeMap(const std::vector<float>& heightmap, int width, int height, uint32_t seed) {
    m_width = width;
    m_height = height;
    m_biomeMap.resize(width * height);
    m_distanceToWaterMap.resize(width * height);

    // First pass: calculate distance to water for all cells
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            float h = heightmap[idx];
            m_distanceToWaterMap[idx] = calculateDistanceToWater(heightmap, x, y, width, height, m_waterLevel);
        }
    }

    // Second pass: determine biomes based on environmental factors
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> localNoise(-0.1f, 0.1f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            BiomeCell& cell = m_biomeMap[idx];

            float h = heightmap[idx];
            float slope = calculateSlope(heightmap, x, y, width);
            float latitude = (static_cast<float>(y) / height) * 2.0f - 1.0f; // -1 to 1

            // PHASE 11 AGENT 3: Add multi-scale patch noise for biome variety
            // This creates distinct biome patches instead of uniform climate zones
            float patchNoise = generatePatchNoise(static_cast<float>(x), static_cast<float>(y), seed);

            // Local fine-grain variation
            float fineNoise = localNoise(rng);

            // Apply patch noise to temperature and moisture with increased strength
            // This forces multiple distinct biomes per island instead of mono-biome collapse
            float tempOffset = patchNoise * 0.6f + fineNoise;  // Increased from 0.1 to 0.6
            float moistOffset = generatePatchNoise(static_cast<float>(x) + 1000.0f, static_cast<float>(y), seed + 7919) * 0.5f + fineNoise;

            float temp = calculateTemperature(h, latitude, tempOffset);
            float moist = calculateMoisture(h, m_distanceToWaterMap[idx], moistOffset);

            // Store environmental factors
            cell.elevation = h;
            cell.slope = slope;
            cell.temperature = temp;
            cell.moisture = moist;

            // Determine primary biome
            cell.primaryBiome = determineBiome(h, slope, temp, moist, m_distanceToWaterMap[idx]);
            cell.secondaryBiome = cell.primaryBiome;
            cell.blendFactor = 0.0f;

            // Get properties and set cell values
            const BiomeProperties& props = getProperties(cell.primaryBiome);
            cell.color = props.baseColor;
            cell.fertility = props.fertility;
        }
    }

    // PHASE 11 AGENT 3: Reduced smoothing iterations to preserve biome patches
    // Changed from 2 to 1 to maintain more distinct biome boundaries
    smoothTransitions(1);
    calculateBlendFactors();
}

void BiomeSystem::generateFromIslandData(const struct IslandData& islandData) {
    // Validate island data
    if (islandData.heightmap.empty()) {
        std::cerr << "ERROR: BiomeSystem::generateFromIslandData called with empty heightmap!" << std::endl;
        assert(false && "BiomeSystem::generateFromIslandData - IslandData has empty heightmap");
        return;
    }

    if (islandData.width <= 0 || islandData.height <= 0) {
        std::cerr << "ERROR: BiomeSystem::generateFromIslandData called with invalid dimensions: "
                  << islandData.width << "x" << islandData.height << std::endl;
        assert(false && "BiomeSystem::generateFromIslandData - IslandData has invalid dimensions");
        return;
    }

    // Delegate to the fully implemented generateBiomeMap
    // IslandData contains everything needed: heightmap, dimensions, and seed from params
    generateBiomeMap(islandData.heightmap, islandData.width, islandData.height, islandData.params.seed);

    // Post-process with additional island-specific data if available:
    // Mark cave entrances from IslandData
    for (const auto& cave : islandData.caveEntrances) {
        int cx = static_cast<int>(cave.position.x / (m_worldScale / m_width));
        int cy = static_cast<int>(cave.position.z / (m_worldScale / m_height));
        if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
            int idx = cy * m_width + cx;
            m_biomeMap[idx].primaryBiome = BiomeType::CAVE_ENTRANCE;
            m_biomeMap[idx].color = getProperties(BiomeType::CAVE_ENTRANCE).baseColor;
        }
    }

    // Mark lake shores from IslandData
    for (const auto& lake : islandData.lakes) {
        int lx = static_cast<int>(lake.center.x / (m_worldScale / m_width));
        int ly = static_cast<int>(lake.center.y / (m_worldScale / m_height));
        int radius = static_cast<int>(lake.radius / (m_worldScale / m_width));

        // Mark perimeter as lake shore
        for (int dy = -radius - 1; dy <= radius + 1; ++dy) {
            for (int dx = -radius - 1; dx <= radius + 1; ++dx) {
                int px = lx + dx;
                int py = ly + dy;
                if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
                    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    // Mark cells near the lake edge as lake shore
                    if (dist >= radius - 1 && dist <= radius + 1) {
                        int idx = py * m_width + px;
                        if (m_biomeMap[idx].primaryBiome != BiomeType::SHALLOW_WATER &&
                            m_biomeMap[idx].primaryBiome != BiomeType::OCEAN) {
                            m_biomeMap[idx].primaryBiome = BiomeType::LAKE_SHORE;
                            m_biomeMap[idx].color = getProperties(BiomeType::LAKE_SHORE).baseColor;
                        }
                    }
                }
            }
        }
    }

    // Mark river banks from IslandData
    for (const auto& river : islandData.rivers) {
        // Use river start position (RiverSegment has start/end, not position)
        int rx = static_cast<int>(river.start.x / (m_worldScale / m_width));
        int ry = static_cast<int>(river.start.y / (m_worldScale / m_height));
        int halfWidth = static_cast<int>(river.width / 2 / (m_worldScale / m_width)) + 1;

        for (int dy = -halfWidth; dy <= halfWidth; ++dy) {
            for (int dx = -halfWidth; dx <= halfWidth; ++dx) {
                int px = rx + dx;
                int py = ry + dy;
                if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
                    int idx = py * m_width + px;
                    if (m_biomeMap[idx].primaryBiome != BiomeType::SHALLOW_WATER &&
                        m_biomeMap[idx].primaryBiome != BiomeType::OCEAN) {
                        m_biomeMap[idx].primaryBiome = BiomeType::RIVER_BANK;
                        m_biomeMap[idx].color = getProperties(BiomeType::RIVER_BANK).baseColor;
                    }
                }
            }
        }
    }

    // Re-smooth transitions after adding island-specific features
    smoothTransitions(1);
    calculateBlendFactors();
}

BiomeType BiomeSystem::determineBiome(float height, float slope, float temperature, float moisture, float distanceToWater) const {
    // Water biomes based on depth
    if (height < m_waterLevel - 0.20f) {
        return BiomeType::DEEP_OCEAN;
    }
    if (height < m_waterLevel - 0.10f) {
        // Check for special underwater biomes
        if (temperature > 0.5f && moisture > 0.8f) {
            return BiomeType::CORAL_REEF;
        }
        if (temperature < 0.3f) {
            return BiomeType::KELP_FOREST;
        }
        return BiomeType::OCEAN;
    }
    if (height < m_waterLevel) {
        return BiomeType::SHALLOW_WATER;
    }

    // Coastal biomes
    if (height < m_waterLevel + 0.05f) {
        if (slope > 0.3f) {
            return BiomeType::BEACH_ROCKY;
        }
        if (temperature > 0.6f && moisture > 0.7f) {
            return BiomeType::MANGROVE;
        }
        if (moisture > 0.75f) {
            return BiomeType::SALT_MARSH;
        }
        return BiomeType::BEACH_SANDY;
    }

    // Very high elevation - glaciers and peaks
    if (height > 0.85f) {
        if (temperature < -0.3f) {
            return BiomeType::GLACIER;
        }
        if (slope > 0.5f) {
            return BiomeType::ROCKY_HIGHLANDS;
        }
        return BiomeType::ALPINE_MEADOW;
    }

    // High elevation
    if (height > 0.70f) {
        if (slope > 0.6f) {
            return BiomeType::ROCKY_HIGHLANDS;
        }
        if (temperature < -0.4f) {
            return BiomeType::TUNDRA;
        }
        if (temperature < 0.0f) {
            return BiomeType::BOREAL_FOREST;
        }
        return BiomeType::MOUNTAIN_FOREST;
    }

    // Mid-high elevation
    if (height > 0.55f) {
        if (slope > 0.5f) {
            return BiomeType::ROCKY_HIGHLANDS;
        }
        if (temperature < -0.2f) {
            return BiomeType::BOREAL_FOREST;
        }
        if (moisture < 0.3f) {
            return BiomeType::SHRUBLAND;
        }
        return BiomeType::MOUNTAIN_FOREST;
    }

    // Mid elevation biomes - based on temperature and moisture
    if (height > 0.40f) {
        // Hot and dry - deserts
        if (temperature > 0.6f && moisture < 0.2f) {
            return BiomeType::DESERT_HOT;
        }

        // Cold and dry - cold desert
        if (temperature < -0.1f && moisture < 0.25f) {
            return BiomeType::DESERT_COLD;
        }

        // Hot and wet - tropical
        if (temperature > 0.7f && moisture > 0.7f) {
            return BiomeType::TROPICAL_RAINFOREST;
        }

        // Hot and moderate moisture - savanna
        if (temperature > 0.5f && moisture < 0.5f) {
            return BiomeType::SAVANNA;
        }

        // Cold - boreal/tundra
        if (temperature < -0.3f) {
            return BiomeType::TUNDRA;
        }
        if (temperature < 0.0f && moisture > 0.5f) {
            return BiomeType::BOREAL_FOREST;
        }

        // Wet areas near water
        if (moisture > 0.85f && distanceToWater < 0.1f) {
            return BiomeType::SWAMP;
        }
        if (moisture > 0.8f) {
            return BiomeType::WETLAND;
        }

        // Temperate
        if (moisture > 0.5f) {
            return BiomeType::TEMPERATE_FOREST;
        }

        if (moisture > 0.3f) {
            return BiomeType::SHRUBLAND;
        }

        return BiomeType::GRASSLAND;
    }

    // Low elevation - near water level
    if (moisture > 0.85f) {
        if (distanceToWater < 0.08f) {
            return BiomeType::SWAMP;
        }
        return BiomeType::WETLAND;
    }

    if (temperature > 0.7f && moisture > 0.6f) {
        return BiomeType::TROPICAL_RAINFOREST;
    }

    if (temperature > 0.5f && moisture < 0.4f) {
        return BiomeType::SAVANNA;
    }

    if (moisture > 0.5f) {
        return BiomeType::TEMPERATE_FOREST;
    }

    return BiomeType::GRASSLAND;
}

float BiomeSystem::calculateTemperature(float height, float latitude, float localVariation) const {
    // Base temperature from latitude (equator is hot, poles are cold)
    float latitudeTemp = 1.0f - std::abs(latitude) * 1.5f;
    latitudeTemp = glm::clamp(latitudeTemp, -1.0f, 1.0f);

    // Apply climate zone modifiers
    for (const auto& zone : m_climateZones) {
        if (latitude >= zone.latitudeStart && latitude <= zone.latitudeEnd) {
            float zoneInfluence = 1.0f - std::min(
                std::abs(latitude - zone.latitudeStart),
                std::abs(latitude - zone.latitudeEnd)
            ) / (zone.latitudeEnd - zone.latitudeStart);
            latitudeTemp = glm::mix(latitudeTemp, zone.baseTemperature, zoneInfluence * 0.5f);
        }
    }

    // Altitude cooling effect (higher = colder)
    float altitudeModifier = 0.0f;
    if (height > m_waterLevel) {
        float normalizedAlt = (height - m_waterLevel) / (1.0f - m_waterLevel);
        altitudeModifier = -normalizedAlt * 1.2f; // Significant cooling at altitude
    }

    // Water has moderating effect
    if (height < m_waterLevel) {
        latitudeTemp *= 0.7f; // Water moderates temperature
    }

    float temp = latitudeTemp + altitudeModifier + localVariation;
    return glm::clamp(temp, -1.0f, 1.0f);
}

float BiomeSystem::calculateMoisture(float height, float distanceToWater, float windExposure) const {
    // Base moisture from distance to water
    float baseMoisture = 1.0f - glm::clamp(distanceToWater * 3.0f, 0.0f, 1.0f);

    // Underwater is always wet
    if (height < m_waterLevel) {
        return 1.0f;
    }

    // Altitude affects moisture (can trap moisture at certain elevations)
    float normalizedAlt = (height - m_waterLevel) / (1.0f - m_waterLevel);
    float altitudeModifier = 0.0f;

    // Rain shadow effect at high altitudes
    if (normalizedAlt > 0.5f) {
        altitudeModifier = -0.3f * (normalizedAlt - 0.5f) * 2.0f;
    }
    // Coastal/low areas get more moisture
    else if (normalizedAlt < 0.2f) {
        altitudeModifier = 0.2f * (1.0f - normalizedAlt / 0.2f);
    }

    // Wind exposure variation
    float moisture = baseMoisture + altitudeModifier + windExposure * 0.1f;
    return glm::clamp(moisture, 0.0f, 1.0f);
}

float BiomeSystem::calculateSlope(const std::vector<float>& heightmap, int x, int y, int width) const {
    int height = static_cast<int>(heightmap.size()) / width;

    // Sample neighboring heights
    float center = heightmap[y * width + x];

    auto getHeight = [&](int px, int py) -> float {
        px = glm::clamp(px, 0, width - 1);
        py = glm::clamp(py, 0, height - 1);
        return heightmap[py * width + px];
    };

    // Sobel-like gradient calculation
    float dx = (getHeight(x + 1, y) - getHeight(x - 1, y)) * 0.5f;
    float dy = (getHeight(x, y + 1) - getHeight(x, y - 1)) * 0.5f;

    // Calculate slope magnitude (0 = flat, 1 = vertical)
    float gradient = std::sqrt(dx * dx + dy * dy);

    // Scale to 0-1 range (adjust multiplier based on terrain scale)
    return glm::clamp(gradient * 10.0f, 0.0f, 1.0f);
}

float BiomeSystem::calculateDistanceToWater(const std::vector<float>& heightmap, int x, int y, int width, int height, float waterLevel) const {
    // Simple flood-fill distance calculation
    // For large maps, this should use a distance transform algorithm

    float currentHeight = heightmap[y * width + x];
    if (currentHeight < waterLevel) {
        return 0.0f; // Already in water
    }

    // BFS to find nearest water
    float maxSearchDist = 50.0f; // Maximum search distance in cells
    float minDist = maxSearchDist;

    // Sample in expanding rings for efficiency
    for (int radius = 1; radius < static_cast<int>(maxSearchDist); ++radius) {
        bool foundWater = false;

        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                // Only check perimeter of current radius
                if (std::abs(dx) != radius && std::abs(dy) != radius) continue;

                int nx = x + dx;
                int ny = y + dy;

                if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

                float neighborHeight = heightmap[ny * width + nx];
                if (neighborHeight < waterLevel) {
                    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (dist < minDist) {
                        minDist = dist;
                        foundWater = true;
                    }
                }
            }
        }

        if (foundWater) break;
    }

    // Normalize distance
    return minDist / maxSearchDist;
}

void BiomeSystem::smoothTransitions(int iterations) {
    if (m_width == 0 || m_height == 0) return;

    std::vector<BiomeCell> tempMap = m_biomeMap;

    for (int iter = 0; iter < iterations; ++iter) {
        for (int y = 1; y < m_height - 1; ++y) {
            for (int x = 1; x < m_width - 1; ++x) {
                int idx = y * m_width + x;
                BiomeCell& cell = tempMap[idx];

                // Count neighboring biome types
                std::unordered_map<BiomeType, int> neighborCounts;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nidx = (y + dy) * m_width + (x + dx);
                        neighborCounts[m_biomeMap[nidx].primaryBiome]++;
                    }
                }

                // Find most common neighbor (excluding self)
                BiomeType mostCommon = cell.primaryBiome;
                int maxCount = 0;
                for (const auto& pair : neighborCounts) {
                    if (pair.second > maxCount && pair.first != cell.primaryBiome) {
                        maxCount = pair.second;
                        mostCommon = pair.first;
                    }
                }

                // PHASE 11 AGENT 3: Increased threshold to preserve more biome variety
                // Changed from 5 to 7 - only replace truly isolated single cells
                // This prevents aggressive biome homogenization
                if (maxCount >= 7) {
                    // Check if this is an isolated cell - replace it
                    int selfCount = neighborCounts[cell.primaryBiome];
                    if (selfCount == 0) {  // Only if completely surrounded (changed from <= 2)
                        cell.primaryBiome = mostCommon;
                        const BiomeProperties& props = getProperties(mostCommon);
                        cell.color = props.baseColor;
                        cell.fertility = props.fertility;
                    }
                }
            }
        }

        m_biomeMap = tempMap;
    }
}

void BiomeSystem::calculateBlendFactors() {
    if (m_width == 0 || m_height == 0) return;

    for (int y = 1; y < m_height - 1; ++y) {
        for (int x = 1; x < m_width - 1; ++x) {
            int idx = y * m_width + x;
            BiomeCell& cell = m_biomeMap[idx];

            // Check for different biomes in neighbors
            BiomeType differentNeighbor = cell.primaryBiome;
            float minDistToDifferent = 1000.0f;

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;

                    int nidx = (y + dy) * m_width + (x + dx);
                    if (m_biomeMap[nidx].primaryBiome != cell.primaryBiome) {
                        float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                        if (dist < minDistToDifferent) {
                            minDistToDifferent = dist;
                            differentNeighbor = m_biomeMap[nidx].primaryBiome;
                        }
                    }
                }
            }

            if (differentNeighbor != cell.primaryBiome) {
                cell.secondaryBiome = differentNeighbor;
                // Blend factor based on proximity (immediate neighbor = 0.5 max blend)
                cell.blendFactor = glm::clamp(0.5f / minDistToDifferent, 0.0f, 0.5f);

                // Update color with blending
                cell.color = blendBiomeColors(cell.primaryBiome, cell.secondaryBiome, cell.blendFactor);
            }
        }
    }
}

glm::vec3 BiomeSystem::blendBiomeColors(BiomeType primary, BiomeType secondary, float factor) const {
    const BiomeProperties& props1 = getProperties(primary);
    const BiomeProperties& props2 = getProperties(secondary);
    return glm::mix(props1.baseColor, props2.baseColor, factor);
}

BiomeQuery BiomeSystem::queryBiome(float worldX, float worldZ) const {
    // Convert world coordinates to biome map coordinates
    float u = (worldX / m_worldScale + 0.5f);
    float v = (worldZ / m_worldScale + 0.5f);
    return queryBiomeNormalized(u, v);
}

BiomeQuery BiomeSystem::queryBiomeNormalized(float u, float v) const {
    BiomeQuery result;

    // Clamp coordinates
    u = glm::clamp(u, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);

    // Get cell coordinates
    int x = static_cast<int>(u * (m_width - 1));
    int y = static_cast<int>(v * (m_height - 1));

    x = glm::clamp(x, 0, m_width - 1);
    y = glm::clamp(y, 0, m_height - 1);

    const BiomeCell& cell = getCell(x, y);
    result.biome = cell.primaryBiome;
    result.properties = getProperties(cell.primaryBiome);
    result.color = cell.color;
    result.blendFactor = cell.blendFactor;

    // Get neighboring biomes for smooth sampling
    auto safeGetBiome = [this](int px, int py) -> BiomeType {
        px = glm::clamp(px, 0, m_width - 1);
        py = glm::clamp(py, 0, m_height - 1);
        return m_biomeMap[py * m_width + px].primaryBiome;
    };

    result.neighbors[0] = safeGetBiome(x - 1, y);     // Left
    result.neighbors[1] = safeGetBiome(x + 1, y);     // Right
    result.neighbors[2] = safeGetBiome(x, y - 1);     // Up
    result.neighbors[3] = safeGetBiome(x, y + 1);     // Down

    // Calculate bilinear weights for smooth interpolation
    float fx = u * (m_width - 1) - x;
    float fy = v * (m_height - 1) - y;

    result.neighborWeights[0] = (1.0f - fx) * 0.25f;
    result.neighborWeights[1] = fx * 0.25f;
    result.neighborWeights[2] = (1.0f - fy) * 0.25f;
    result.neighborWeights[3] = fy * 0.25f;

    return result;
}

const BiomeCell& BiomeSystem::getCell(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return m_defaultCell;
    }
    return m_biomeMap[y * m_width + x];
}

BiomeType BiomeSystem::getBiomeAt(int x, int y) const {
    return getCell(x, y).primaryBiome;
}

const BiomeProperties& BiomeSystem::getProperties(BiomeType type) const {
    auto it = m_properties.find(type);
    if (it != m_properties.end()) {
        return it->second;
    }
    // Return grassland as default fallback
    static BiomeProperties defaultProps;
    defaultProps.type = BiomeType::GRASSLAND;
    defaultProps.name = "Unknown";
    defaultProps.baseColor = glm::vec3(0.5f);
    return defaultProps;
}

BiomeProperties& BiomeSystem::getMutableProperties(BiomeType type) {
    return m_properties[type];
}

void BiomeSystem::setBaseColor(BiomeType type, const glm::vec3& color) {
    if (m_properties.find(type) != m_properties.end()) {
        m_properties[type].baseColor = color;
    }
}

void BiomeSystem::setAccentColor(BiomeType type, const glm::vec3& color) {
    if (m_properties.find(type) != m_properties.end()) {
        m_properties[type].accentColor = color;
    }
}

void BiomeSystem::applyColorShift(const glm::vec3& hueShift) {
    // Apply HSV shift to all biome colors
    auto rgbToHsv = [](const glm::vec3& rgb) -> glm::vec3 {
        float maxC = std::max({rgb.r, rgb.g, rgb.b});
        float minC = std::min({rgb.r, rgb.g, rgb.b});
        float delta = maxC - minC;

        glm::vec3 hsv;
        hsv.z = maxC; // Value

        if (delta < 0.00001f) {
            hsv.x = 0.0f;
            hsv.y = 0.0f;
            return hsv;
        }

        hsv.y = delta / maxC; // Saturation

        if (rgb.r >= maxC) {
            hsv.x = (rgb.g - rgb.b) / delta;
        } else if (rgb.g >= maxC) {
            hsv.x = 2.0f + (rgb.b - rgb.r) / delta;
        } else {
            hsv.x = 4.0f + (rgb.r - rgb.g) / delta;
        }

        hsv.x /= 6.0f;
        if (hsv.x < 0.0f) hsv.x += 1.0f;

        return hsv;
    };

    auto hsvToRgb = [](const glm::vec3& hsv) -> glm::vec3 {
        if (hsv.y <= 0.0f) {
            return glm::vec3(hsv.z);
        }

        float h = hsv.x * 6.0f;
        int i = static_cast<int>(h);
        float f = h - i;
        float p = hsv.z * (1.0f - hsv.y);
        float q = hsv.z * (1.0f - hsv.y * f);
        float t = hsv.z * (1.0f - hsv.y * (1.0f - f));

        switch (i % 6) {
            case 0: return glm::vec3(hsv.z, t, p);
            case 1: return glm::vec3(q, hsv.z, p);
            case 2: return glm::vec3(p, hsv.z, t);
            case 3: return glm::vec3(p, q, hsv.z);
            case 4: return glm::vec3(t, p, hsv.z);
            default: return glm::vec3(hsv.z, p, q);
        }
    };

    for (auto& pair : m_properties) {
        // Apply shift to base color
        glm::vec3 hsv = rgbToHsv(pair.second.baseColor);
        hsv.x = std::fmod(hsv.x + hueShift.x, 1.0f);
        hsv.y = glm::clamp(hsv.y * (1.0f + hueShift.y), 0.0f, 1.0f);
        hsv.z = glm::clamp(hsv.z * (1.0f + hueShift.z), 0.0f, 1.0f);
        pair.second.baseColor = hsvToRgb(hsv);

        // Apply shift to accent color
        hsv = rgbToHsv(pair.second.accentColor);
        hsv.x = std::fmod(hsv.x + hueShift.x, 1.0f);
        hsv.y = glm::clamp(hsv.y * (1.0f + hueShift.y), 0.0f, 1.0f);
        hsv.z = glm::clamp(hsv.z * (1.0f + hueShift.z), 0.0f, 1.0f);
        pair.second.accentColor = hsvToRgb(hsv);
    }

    // Update biome map colors
    for (auto& cell : m_biomeMap) {
        cell.color = blendBiomeColors(cell.primaryBiome, cell.secondaryBiome, cell.blendFactor);
    }
}

void BiomeSystem::setClimateZones(const std::vector<ClimateZone>& zones) {
    m_climateZones = zones;
}

float BiomeSystem::getTemperature(float x, float y) const {
    BiomeQuery query = queryBiome(x, y);
    int cellX = static_cast<int>((x / m_worldScale + 0.5f) * (m_width - 1));
    int cellY = static_cast<int>((y / m_worldScale + 0.5f) * (m_height - 1));
    const BiomeCell& cell = getCell(cellX, cellY);
    return cell.temperature;
}

float BiomeSystem::getMoisture(float x, float y) const {
    int cellX = static_cast<int>((x / m_worldScale + 0.5f) * (m_width - 1));
    int cellY = static_cast<int>((y / m_worldScale + 0.5f) * (m_height - 1));
    const BiomeCell& cell = getCell(cellX, cellY);
    return cell.moisture;
}

// Vegetation queries
float BiomeSystem::getTreeDensity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.treeDensity;
}

float BiomeSystem::getGrassDensity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.grassDensity;
}

float BiomeSystem::getShrubDensity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.shrubDensity;
}

bool BiomeSystem::canPlaceTree(float worldX, float worldZ, float& outScale) const {
    BiomeQuery query = queryBiome(worldX, worldZ);

    // Check if biome supports trees
    if (query.properties.treeDensity < 0.05f) {
        return false;
    }

    // Check if underwater
    if (isAquaticBiome(query.biome)) {
        return false;
    }

    // Calculate scale based on biome properties
    float baseScale = 1.0f;

    switch (query.biome) {
        case BiomeType::TROPICAL_RAINFOREST:
            baseScale = 1.3f; // Larger trees
            break;
        case BiomeType::TEMPERATE_FOREST:
            baseScale = 1.1f;
            break;
        case BiomeType::BOREAL_FOREST:
            baseScale = 0.9f; // Smaller coniferous trees
            break;
        case BiomeType::MOUNTAIN_FOREST:
            baseScale = 0.8f;
            break;
        case BiomeType::MANGROVE:
            baseScale = 0.7f;
            break;
        case BiomeType::SAVANNA:
            baseScale = 1.2f; // Acacia-style trees
            break;
        default:
            baseScale = 1.0f;
            break;
    }

    // Modify scale by temperature and moisture
    float moistureModifier = 0.7f + query.properties.moisture * 0.6f;
    float tempModifier = 0.8f + (query.properties.temperature + 1.0f) * 0.2f;

    outScale = baseScale * moistureModifier * tempModifier;
    outScale = glm::clamp(outScale, 0.3f, 2.0f);

    return true;
}

// Wildlife queries
float BiomeSystem::getHerbivoreCapacity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.herbivoreCapacity;
}

float BiomeSystem::getCarnivoreCapacity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.carnivoreCapacity;
}

float BiomeSystem::getAquaticCapacity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.aquaticCapacity;
}

float BiomeSystem::getFlyingCapacity(float worldX, float worldZ) const {
    BiomeQuery query = queryBiome(worldX, worldZ);
    return query.properties.flyingCapacity;
}

// Biome transitions
void BiomeSystem::addTransition(BiomeType from, BiomeType to, float blendWidth, BiomeType intermediate) {
    BiomeTransition transition;
    transition.from = from;
    transition.to = to;
    transition.blendWidth = blendWidth;
    transition.transitionBiome = intermediate;
    m_transitions.push_back(transition);

    // Add reverse transition as well
    BiomeTransition reverse;
    reverse.from = to;
    reverse.to = from;
    reverse.blendWidth = blendWidth;
    reverse.transitionBiome = intermediate;
    m_transitions.push_back(reverse);
}

float BiomeSystem::getTransitionFactor(BiomeType from, BiomeType to, float distance) const {
    // Find the transition definition
    for (const auto& trans : m_transitions) {
        if (trans.from == from && trans.to == to) {
            if (distance >= trans.blendWidth) {
                return 1.0f;
            }
            // Smooth hermite interpolation
            float t = distance / trans.blendWidth;
            return t * t * (3.0f - 2.0f * t);
        }
    }
    // Default transition if not defined
    float defaultWidth = 0.1f;
    if (distance >= defaultWidth) {
        return 1.0f;
    }
    float t = distance / defaultWidth;
    return t * t * (3.0f - 2.0f * t);
}

// Serialization
void BiomeSystem::serialize(std::vector<uint8_t>& data) const {
    // Calculate total size needed
    size_t headerSize = sizeof(int) * 3 + sizeof(float) * 2; // width, height, numCells, worldScale, waterLevel
    size_t cellSize = sizeof(uint8_t) * 2 + sizeof(float) * 8; // biomes + environmental data
    size_t totalSize = headerSize + m_biomeMap.size() * cellSize;

    data.resize(totalSize);
    uint8_t* ptr = data.data();

    // Write header
    std::memcpy(ptr, &m_width, sizeof(int)); ptr += sizeof(int);
    std::memcpy(ptr, &m_height, sizeof(int)); ptr += sizeof(int);
    int numCells = static_cast<int>(m_biomeMap.size());
    std::memcpy(ptr, &numCells, sizeof(int)); ptr += sizeof(int);
    std::memcpy(ptr, &m_worldScale, sizeof(float)); ptr += sizeof(float);
    std::memcpy(ptr, &m_waterLevel, sizeof(float)); ptr += sizeof(float);

    // Write biome cells
    for (const auto& cell : m_biomeMap) {
        uint8_t primary = static_cast<uint8_t>(cell.primaryBiome);
        uint8_t secondary = static_cast<uint8_t>(cell.secondaryBiome);
        std::memcpy(ptr, &primary, sizeof(uint8_t)); ptr += sizeof(uint8_t);
        std::memcpy(ptr, &secondary, sizeof(uint8_t)); ptr += sizeof(uint8_t);
        std::memcpy(ptr, &cell.blendFactor, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.temperature, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.moisture, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.elevation, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.slope, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.color.x, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.color.y, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &cell.color.z, sizeof(float)); ptr += sizeof(float);
    }
}

void BiomeSystem::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(int) * 3 + sizeof(float) * 2) {
        return; // Invalid data
    }

    const uint8_t* ptr = data.data();

    // Read header
    std::memcpy(&m_width, ptr, sizeof(int)); ptr += sizeof(int);
    std::memcpy(&m_height, ptr, sizeof(int)); ptr += sizeof(int);
    int numCells;
    std::memcpy(&numCells, ptr, sizeof(int)); ptr += sizeof(int);
    std::memcpy(&m_worldScale, ptr, sizeof(float)); ptr += sizeof(float);
    std::memcpy(&m_waterLevel, ptr, sizeof(float)); ptr += sizeof(float);

    // Validate and resize
    if (numCells != m_width * m_height || numCells <= 0) {
        return; // Invalid data
    }

    m_biomeMap.resize(numCells);

    // Read biome cells
    for (int i = 0; i < numCells; ++i) {
        BiomeCell& cell = m_biomeMap[i];
        uint8_t primary, secondary;
        std::memcpy(&primary, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
        std::memcpy(&secondary, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
        cell.primaryBiome = static_cast<BiomeType>(primary);
        cell.secondaryBiome = static_cast<BiomeType>(secondary);
        std::memcpy(&cell.blendFactor, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.temperature, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.moisture, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.elevation, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.slope, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.color.x, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.color.y, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&cell.color.z, ptr, sizeof(float)); ptr += sizeof(float);

        // Restore fertility from properties
        cell.fertility = getProperties(cell.primaryBiome).fertility;
    }

    // Rebuild distance to water map
    m_distanceToWaterMap.resize(numCells, 0.0f);
}

// Debug/visualization
std::vector<glm::vec3> BiomeSystem::generateBiomeColorMap() const {
    std::vector<glm::vec3> colorMap;
    colorMap.reserve(m_biomeMap.size());

    for (const auto& cell : m_biomeMap) {
        colorMap.push_back(cell.color);
    }

    return colorMap;
}

std::string BiomeSystem::getBiomeName(BiomeType type) const {
    auto it = m_properties.find(type);
    if (it != m_properties.end()) {
        return it->second.name;
    }
    return biomeToString(type);
}

// ============================================================================
// PHASE 11 AGENT 3: Patch Noise and Biome Diversity
// ============================================================================

// Simple hash function for noise generation
static uint32_t hash(uint32_t x, uint32_t y, uint32_t seed) {
    uint32_t h = seed;
    h ^= x * 0x85ebca6b;
    h ^= y * 0xc2b2ae35;
    h ^= h >> 16;
    h *= 0x7feb352d;
    h ^= h >> 15;
    h *= 0x846ca68b;
    h ^= h >> 16;
    return h;
}

// Smooth interpolation (smoothstep)
static float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

// Perlin-style noise implementation
float BiomeSystem::perlinNoise(float x, float y, uint32_t seed) const {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = x - x0;
    float fy = y - y0;

    // Hash corner coordinates to get pseudo-random values
    float h00 = static_cast<float>(hash(x0, y0, seed) & 0xFFFF) / 65535.0f;
    float h10 = static_cast<float>(hash(x1, y0, seed) & 0xFFFF) / 65535.0f;
    float h01 = static_cast<float>(hash(x0, y1, seed) & 0xFFFF) / 65535.0f;
    float h11 = static_cast<float>(hash(x1, y1, seed) & 0xFFFF) / 65535.0f;

    // Bilinear interpolation with smoothstep
    float sx = smoothstep(fx);
    float sy = smoothstep(fy);

    float nx0 = glm::mix(h00, h10, sx);
    float nx1 = glm::mix(h01, h11, sx);

    return glm::mix(nx0, nx1, sy) * 2.0f - 1.0f; // Range -1 to 1
}

// Multi-octave patch noise for biome variety
float BiomeSystem::generatePatchNoise(float x, float y, uint32_t seed) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.02f;  // Low frequency = large patches
    float totalAmplitude = 0.0f;

    // 3 octaves for natural-looking patches
    for (int octave = 0; octave < 3; ++octave) {
        result += perlinNoise(x * frequency, y * frequency, seed + octave * 1000) * amplitude;
        totalAmplitude += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return result / totalAmplitude;  // Normalize to -1 to 1 range
}

// Calculate biome diversity metrics using flood-fill for patch detection
BiomeSystem::BiomeDiversityMetrics BiomeSystem::calculateDiversityMetrics() const {
    BiomeDiversityMetrics metrics;

    if (m_biomeMap.empty()) {
        return metrics;
    }

    // Count biome occurrences (excluding water biomes for terrestrial diversity)
    std::unordered_map<BiomeType, int> biomeCountMap;
    int terrestrialCells = 0;

    for (const auto& cell : m_biomeMap) {
        BiomeType biome = cell.primaryBiome;

        // Skip deep water biomes for terrestrial diversity calculation
        if (!isAquaticBiome(biome) || biome == BiomeType::SHALLOW_WATER) {
            biomeCountMap[biome]++;
            terrestrialCells++;
        }
    }

    metrics.totalBiomeCount = static_cast<int>(biomeCountMap.size());
    metrics.biomeCounts.reserve(biomeCountMap.size());

    for (const auto& pair : biomeCountMap) {
        metrics.biomeCounts.push_back(pair.second);
    }

    // Calculate Shannon diversity index: H = -Σ(p_i * ln(p_i))
    metrics.diversityIndex = 0.0f;
    if (terrestrialCells > 0) {
        for (int count : metrics.biomeCounts) {
            float p = static_cast<float>(count) / terrestrialCells;
            if (p > 0.0f) {
                metrics.diversityIndex -= p * std::log(p);
            }
        }
    }

    // Find largest contiguous patch using flood fill
    std::vector<bool> visited(m_biomeMap.size(), false);
    metrics.largestPatchSize = 0;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            if (visited[idx]) continue;

            BiomeType biome = m_biomeMap[idx].primaryBiome;
            if (isAquaticBiome(biome) && biome != BiomeType::SHALLOW_WATER) {
                visited[idx] = true;
                continue;
            }

            // Flood fill to find patch size
            std::queue<std::pair<int, int>> queue;
            queue.push({x, y});
            visited[idx] = true;
            int patchSize = 0;

            while (!queue.empty()) {
                auto [cx, cy] = queue.front();
                queue.pop();
                patchSize++;

                // Check 4-connected neighbors
                const int dx[] = {0, 0, -1, 1};
                const int dy[] = {-1, 1, 0, 0};

                for (int i = 0; i < 4; ++i) {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];

                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                        int nidx = ny * m_width + nx;
                        if (!visited[nidx] && m_biomeMap[nidx].primaryBiome == biome) {
                            visited[nidx] = true;
                            queue.push({nx, ny});
                        }
                    }
                }
            }

            metrics.patchSizes.push_back(patchSize);
            if (patchSize > metrics.largestPatchSize) {
                metrics.largestPatchSize = patchSize;
            }
        }
    }

    // Calculate dominance (largest patch as fraction of total)
    if (terrestrialCells > 0) {
        metrics.dominance = static_cast<float>(metrics.largestPatchSize) / terrestrialCells;
    }

    return metrics;
}

// Log diversity metrics for validation
void BiomeSystem::logDiversityMetrics() const {
    auto metrics = calculateDiversityMetrics();

    std::cout << "\n=== BIOME DIVERSITY METRICS (PHASE 11 AGENT 3) ===" << std::endl;
    std::cout << "Total distinct biomes: " << metrics.totalBiomeCount << std::endl;
    std::cout << "Largest patch size: " << metrics.largestPatchSize << " cells" << std::endl;
    std::cout << "Total patches: " << metrics.patchSizes.size() << std::endl;
    std::cout << "Shannon diversity index: " << std::fixed << std::setprecision(3)
              << metrics.diversityIndex << std::endl;
    std::cout << "Dominance (largest patch ratio): " << std::fixed << std::setprecision(3)
              << metrics.dominance << std::endl;

    // Log biome distribution
    std::unordered_map<BiomeType, int> biomeCountMap;
    int totalCells = 0;
    for (const auto& cell : m_biomeMap) {
        if (!isAquaticBiome(cell.primaryBiome) || cell.primaryBiome == BiomeType::SHALLOW_WATER) {
            biomeCountMap[cell.primaryBiome]++;
            totalCells++;
        }
    }

    std::cout << "\nBiome coverage:" << std::endl;
    std::vector<std::pair<BiomeType, int>> sorted(biomeCountMap.begin(), biomeCountMap.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for (const auto& [biome, count] : sorted) {
        float percentage = (totalCells > 0) ? (100.0f * count / totalCells) : 0.0f;
        std::cout << "  " << std::setw(25) << std::left << biomeToString(biome)
                  << ": " << std::setw(6) << std::right << count
                  << " cells (" << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
    }
    std::cout << "======================================\n" << std::endl;
}
