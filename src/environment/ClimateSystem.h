#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <deque>

// Forward declarations
class Terrain;
class SeasonManager;

// Climate events that can affect the world
enum class ClimateEvent {
    NONE,
    VOLCANIC_WINTER,    // -3°C for 50 game-days
    SOLAR_MAXIMUM,      // +2°C for 100 game-days
    DROUGHT,            // -50% moisture for 30 game-days
    MONSOON,            // +100% moisture for 20 game-days
    ICE_AGE_START,      // Long-term cooling begins
    ICE_AGE_END         // Long-term warming begins
};

// Climate biome types based on Whittaker diagram (temperature vs precipitation)
// Note: This is separate from the terrain-based BiomeType in BiomeSystem.h
enum class ClimateBiome {
    // Water/Coastal
    DEEP_OCEAN,
    SHALLOW_WATER,
    BEACH,

    // Tropical (hot + wet)
    TROPICAL_RAINFOREST,
    TROPICAL_SEASONAL,

    // Temperate
    TEMPERATE_FOREST,
    TEMPERATE_GRASSLAND,

    // Boreal/Cold
    BOREAL_FOREST,
    TUNDRA,
    ICE,

    // Dry
    DESERT_HOT,
    DESERT_COLD,
    SAVANNA,

    // Wetlands
    SWAMP,

    // Elevation-based
    MOUNTAIN_MEADOW,
    MOUNTAIN_ROCK,
    MOUNTAIN_SNOW,

    COUNT
};

// Climate data at a specific position
struct ClimateData {
    float temperature;      // -30 to +40 Celsius (normalized 0-1 internally)
    float moisture;         // 0-1 (desert to rainforest)
    float elevation;        // 0-1 (sea level to mountain peak)
    float slope;            // 0-1 (flat to cliff)
    float distanceToWater;  // World units to nearest water
    float latitude;         // -1 to 1 (south to north)

    // Get biome based on climate factors (Whittaker diagram approach)
    ClimateBiome getBiome() const;

    // Get biome-specific colors for rendering
    glm::vec3 getPrimaryColor() const;
    glm::vec3 getSecondaryColor() const;
};

// Biome blend information for smooth transitions
struct BiomeBlend {
    ClimateBiome primary = ClimateBiome::TEMPERATE_GRASSLAND;
    ClimateBiome secondary = ClimateBiome::TEMPERATE_GRASSLAND;
    float blendFactor = 0.0f;      // 0 = pure primary, 1 = pure secondary
    float noiseOffset = 0.0f;      // For natural-looking boundaries
    bool isTransitioning = false;  // Whether biome is actively changing
};

// Climate grid cell for tracking dynamic changes
struct ClimateGridCell {
    float baseTemperature = 0.5f;    // Original temperature before offsets
    float currentTemperature = 0.5f; // Current temperature with all modifiers
    float baseMoisture = 0.5f;       // Original moisture
    float currentMoisture = 0.5f;    // Current moisture with modifiers
    ClimateBiome primaryBiome = ClimateBiome::TEMPERATE_GRASSLAND;
    ClimateBiome previousBiome = ClimateBiome::TEMPERATE_GRASSLAND;
    float transitionProgress = 0.0f; // 0-1 for biome transition
    bool isTransitioning = false;
};

// Vegetation density parameters per biome
struct VegetationDensity {
    float treeDensity;      // 0-1
    float grassDensity;     // 0-1
    float flowerDensity;    // 0-1
    float shrubDensity;     // 0-1
    float fernDensity;      // 0-1
    float cactusDensity;    // 0-1

    static VegetationDensity forBiome(ClimateBiome biome);
};

class ClimateSystem {
public:
    ClimateSystem();
    ~ClimateSystem() = default;

    // Initialize with terrain reference
    void initialize(const Terrain* terrain, SeasonManager* seasonMgr);

    // Update climate simulation (call each frame for dynamic weather)
    void update(float deltaTime);

    // Get climate data at a world position
    ClimateData getClimateAt(const glm::vec3& worldPos) const;
    ClimateData getClimateAt(float x, float z) const;

    // Get biome blend for smooth transitions
    BiomeBlend calculateBiomeBlend(const ClimateData& climate) const;

    // Simulate moisture distribution (rain shadows, etc.)
    void simulateMoisture();

    // Calculate optimal vegetation for position
    VegetationDensity getVegetationDensity(const glm::vec3& worldPos) const;

    // World configuration
    void setWorldLatitude(float lat) { worldLatitude = lat; }
    float getWorldLatitude() const { return worldLatitude; }

    void setPrevailingWindDirection(const glm::vec2& dir) { prevailingWind = glm::normalize(dir); }
    glm::vec2 getPrevailingWindDirection() const { return prevailingWind; }

    // Get base temperature modified by season
    float getSeasonalTemperature(float baseTemp) const;

    // Get moisture texture for GPU (if generated)
    const std::vector<float>& getMoistureMap() const { return moistureMap; }

    // Biome name for debug/UI
    static const char* getBiomeName(ClimateBiome biome);

    // Climate event methods
    void triggerRandomEvent();
    void startEvent(ClimateEvent event, float duration);
    void endEvent();
    bool hasActiveEvent() const { return m_activeEvent != ClimateEvent::NONE; }
    ClimateEvent getActiveEvent() const { return m_activeEvent; }
    float getEventTimeRemaining() const { return m_eventTimeRemaining; }
    const char* getEventName() const;

    // Global climate state
    float getGlobalTemperature() const;
    float getGlobalTemperatureOffset() const { return m_globalTemperatureOffset; }
    float getSimulationTime() const { return m_simulationTime; }

    // Temperature history for graphing
    const std::deque<float>& getTemperatureHistory() const { return m_temperatureHistory; }

    // Climate grid access
    const ClimateGridCell* getClimateGridCell(int x, int z) const;

private:
    const Terrain* terrain = nullptr;
    SeasonManager* seasonManager = nullptr;

    // World parameters
    float worldLatitude = 45.0f;        // Degrees (affects base temperature)
    glm::vec2 prevailingWind = {1.0f, 0.0f};  // Primary wind direction
    float baseTemperature = 15.0f;      // Average temperature at sea level

    // Precomputed moisture map (from rain shadow simulation)
    std::vector<float> moistureMap;
    int moistureMapWidth = 0;
    int moistureMapDepth = 0;

    // Climate calculation helpers
    float calculateBaseTemperature(float elevation, float latitude) const;
    float calculateMoisture(float x, float z, float elevation) const;
    float calculateSlope(float x, float z) const;
    float calculateDistanceToWater(float x, float z) const;

    // Biome determination using Whittaker diagram
    ClimateBiome whittakerDiagram(float temperature, float precipitation) const;

    // Dynamic climate update helpers
    void updateGlobalTemperature(float deltaTime);
    void updateMoisturePatterns(float deltaTime);
    void updateBiomeTransitions(float deltaTime);
    void applyClimateEvent(float deltaTime);
    void initializeClimateGrid();
    void recordTemperatureHistory();

    // Dynamic climate state
    float m_simulationTime = 0.0f;
    float m_globalTemperatureOffset = 0.0f;
    float m_iceAgeModifier = 0.0f;          // Long-term ice age effect
    bool m_inIceAge = false;

    // Climate events
    ClimateEvent m_activeEvent = ClimateEvent::NONE;
    float m_eventDuration = 0.0f;
    float m_eventTimeRemaining = 0.0f;
    float m_eventCheckTimer = 0.0f;

    // Climate grid for dynamic tracking
    std::vector<ClimateGridCell> m_climateGrid;
    int m_gridWidth = 0;
    int m_gridHeight = 0;
    float m_gridCellSize = 10.0f;  // World units per grid cell
    bool m_gridInitialized = false;

    // Temperature history for UI graphing
    std::deque<float> m_temperatureHistory;
    float m_historyRecordTimer = 0.0f;
    static constexpr size_t MAX_HISTORY_SIZE = 200;
    static constexpr float HISTORY_RECORD_INTERVAL = 5.0f;  // Record every 5 seconds
};
