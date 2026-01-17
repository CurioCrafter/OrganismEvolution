#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <random>

// Forward declarations
class BiomeSystem;

// Predefined planet theme presets
enum class PlanetPreset {
    EARTH_LIKE,         // Standard Earth colors
    ALIEN_PURPLE,       // Purple/pink vegetation, orange sky
    ALIEN_RED,          // Red vegetation, yellow sky
    ALIEN_BLUE,         // Blue vegetation, green sky
    FROZEN_WORLD,       // Ice and snow dominant
    DESERT_WORLD,       // Arid, orange/brown dominant
    OCEAN_WORLD,        // Mostly water, tropical islands
    VOLCANIC_WORLD,     // Lava, ash, dark rocks
    BIOLUMINESCENT,     // Glowing vegetation, dark atmosphere
    CRYSTAL_WORLD,      // Crystal formations, reflective surfaces
    TOXIC_WORLD,        // Green/yellow toxic atmosphere
    ANCIENT_WORLD,      // Weathered, mossy, ancient ruins vibe
    CUSTOM              // User-defined
};

// Time of day for lighting calculations
enum class TimeOfDay {
    DAWN,
    MORNING,
    NOON,
    AFTERNOON,
    DUSK,
    NIGHT
};

// Atmosphere properties
struct AtmosphereSettings {
    glm::vec3 skyZenithColor;       // Color at top of sky
    glm::vec3 skyHorizonColor;      // Color at horizon
    glm::vec3 sunColor;             // Sun/star color
    glm::vec3 moonColor;            // Moon color (if any)
    glm::vec3 fogColor;             // Distance fog color
    glm::vec3 ambientColor;         // Global ambient light

    float fogDensity;               // How thick the fog is
    float fogStart;                 // Distance fog begins
    float rayleighScattering;       // Atmospheric scattering
    float mieScattering;            // Particle scattering

    float sunIntensity;             // Sun brightness
    float moonIntensity;            // Moon brightness
    float starVisibility;           // How visible stars are

    // Aurora/special effects
    bool hasAurora;
    glm::vec3 auroraColor1;
    glm::vec3 auroraColor2;
    float auroraIntensity;
};

// Terrain color palette
struct TerrainPalette {
    // Water colors
    glm::vec3 deepWaterColor;
    glm::vec3 shallowWaterColor;
    glm::vec3 waterFoamColor;
    glm::vec3 waterReflectionTint;

    // Ground colors
    glm::vec3 sandColor;
    glm::vec3 dirtColor;
    glm::vec3 rockColor;
    glm::vec3 cliffColor;

    // Vegetation base colors (applied to biome system)
    glm::vec3 grassColor;
    glm::vec3 forestColor;
    glm::vec3 jungleColor;
    glm::vec3 shrubColor;

    // Snow/ice
    glm::vec3 snowColor;
    glm::vec3 iceColor;
    glm::vec3 glacierColor;

    // Special
    glm::vec3 lavaColor;
    glm::vec3 ashColor;
    glm::vec3 crystalColor;
    glm::vec3 mossColor;
};

// Vegetation color variations
struct VegetationPalette {
    glm::vec3 treeLeafColor;
    glm::vec3 treeBarkColor;
    glm::vec3 treeLeafVariation;    // Random variation range

    glm::vec3 grassBaseColor;
    glm::vec3 grassTipColor;
    glm::vec3 grassVariation;

    glm::vec3 flowerColors[6];      // Variety of flower colors
    glm::vec3 mushroomColors[4];    // Variety of mushroom colors

    float colorSaturation;          // Overall saturation multiplier
    float colorBrightness;          // Overall brightness multiplier
};

// Creature color influence
struct CreaturePalette {
    glm::vec3 herbivoreBaseTint;
    glm::vec3 carnivoreTint;
    glm::vec3 aquaticTint;
    glm::vec3 flyingTint;

    float environmentInfluence;     // How much environment affects creature colors
    float geneticVariation;         // How much individual variation
};

// Post-processing color grading
struct ColorGrading {
    glm::vec3 colorFilter;          // Overall color tint
    glm::vec3 shadowColor;          // Color of shadows
    glm::vec3 highlightColor;       // Color of highlights

    float contrast;                 // 0-2, 1 is neutral
    float saturation;               // 0-2, 1 is neutral
    float brightness;               // 0-2, 1 is neutral
    float gamma;                    // Gamma correction

    // Split toning
    glm::vec3 splitToneShadows;
    glm::vec3 splitToneHighlights;
    float splitToneBalance;         // -1 to 1

    // Vignette
    float vignetteIntensity;
    float vignetteRadius;
};

// Complete planet theme
struct PlanetThemeData {
    std::string name;
    PlanetPreset preset;
    uint32_t seed;

    AtmosphereSettings atmosphere;
    TerrainPalette terrain;
    VegetationPalette vegetation;
    CreaturePalette creatures;
    ColorGrading colorGrading;

    // Time-of-day variations (interpolated)
    AtmosphereSettings atmosphereDawn;
    AtmosphereSettings atmosphereNoon;
    AtmosphereSettings atmosphereDusk;
    AtmosphereSettings atmosphereNight;
};

// Main planet theme class
class PlanetTheme {
public:
    PlanetTheme();
    ~PlanetTheme();

    // Initialize from preset
    void initializePreset(PlanetPreset preset, uint32_t seed = 0);

    // Generate random alien theme
    void generateRandom(uint32_t seed);

    // Create from explicit data
    void setThemeData(const PlanetThemeData& data);

    // Time of day interpolation
    void setTimeOfDay(float normalizedTime);  // 0-1 where 0.5 is noon
    void setTimeOfDay(TimeOfDay time);
    float getTimeOfDay() const { return m_currentTime; }

    // Get current interpolated atmosphere
    AtmosphereSettings getCurrentAtmosphere() const;

    // Accessors
    const PlanetThemeData& getData() const { return m_data; }
    PlanetThemeData& getMutableData() { return m_data; }
    const TerrainPalette& getTerrain() const { return m_data.terrain; }
    const VegetationPalette& getVegetation() const { return m_data.vegetation; }
    const CreaturePalette& getCreatures() const { return m_data.creatures; }
    const ColorGrading& getColorGrading() const { return m_data.colorGrading; }

    // Color query helpers
    glm::vec3 getWaterColor(float depth) const;
    glm::vec3 getSkyColor(float elevation) const;  // 0 = horizon, 1 = zenith
    glm::vec3 getFogColor() const;
    glm::vec3 getAmbientColor() const;
    glm::vec3 getSunColor() const;

    // Terrain color helpers
    glm::vec3 getTerrainColor(float height, float slope, float moisture) const;
    glm::vec3 blendTerrainColors(const glm::vec3& base, float height) const;

    // Apply theme to systems
    void applyToBiomeSystem(BiomeSystem& biomeSystem) const;

    // Shader constant generation
    struct ShaderConstants {
        glm::vec4 skyZenithColor;
        glm::vec4 skyHorizonColor;
        glm::vec4 sunColor;
        glm::vec4 fogColor;
        glm::vec4 ambientColor;

        glm::vec4 waterDeepColor;
        glm::vec4 waterShallowColor;

        glm::vec4 colorFilter;
        glm::vec4 shadowColor;
        glm::vec4 highlightColor;

        float fogDensity;
        float fogStart;
        float sunIntensity;
        float contrast;

        float saturation;
        float brightness;
        float timeOfDay;
        float padding;
    };

    ShaderConstants getShaderConstants() const;

    // Serialization
    void serialize(std::vector<uint8_t>& data) const;
    void deserialize(const std::vector<uint8_t>& data);

    // Preset generators
    static PlanetThemeData createEarthLike(uint32_t seed);
    static PlanetThemeData createAlienPurple(uint32_t seed);
    static PlanetThemeData createAlienRed(uint32_t seed);
    static PlanetThemeData createAlienBlue(uint32_t seed);
    static PlanetThemeData createFrozenWorld(uint32_t seed);
    static PlanetThemeData createDesertWorld(uint32_t seed);
    static PlanetThemeData createOceanWorld(uint32_t seed);
    static PlanetThemeData createVolcanicWorld(uint32_t seed);
    static PlanetThemeData createBioluminescent(uint32_t seed);
    static PlanetThemeData createCrystalWorld(uint32_t seed);
    static PlanetThemeData createToxicWorld(uint32_t seed);
    static PlanetThemeData createAncientWorld(uint32_t seed);

private:
    // Interpolate between atmosphere settings
    AtmosphereSettings interpolateAtmosphere(const AtmosphereSettings& a, const AtmosphereSettings& b, float t) const;

    // Color utilities
    glm::vec3 hsvToRgb(const glm::vec3& hsv) const;
    glm::vec3 rgbToHsv(const glm::vec3& rgb) const;
    glm::vec3 shiftHue(const glm::vec3& color, float shift) const;
    glm::vec3 adjustSaturation(const glm::vec3& color, float factor) const;
    glm::vec3 adjustBrightness(const glm::vec3& color, float factor) const;

    // Random palette generation
    void generateRandomPalette(std::mt19937& rng);
    glm::vec3 generateHarmoniousColor(std::mt19937& rng, const glm::vec3& baseColor, float hueRange) const;
    glm::vec3 generateComplementaryColor(const glm::vec3& color) const;
    glm::vec3 generateAnalogousColor(const glm::vec3& color, float offset) const;

    PlanetThemeData m_data;
    float m_currentTime = 0.5f;  // Noon by default
};

// Utility functions
const char* presetToString(PlanetPreset preset);
PlanetPreset stringToPreset(const std::string& name);
glm::vec3 temperatureToColor(float temperature);  // For heat maps
