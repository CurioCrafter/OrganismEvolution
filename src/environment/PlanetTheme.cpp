#include "PlanetTheme.h"
#include "BiomeSystem.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Constructor
PlanetTheme::PlanetTheme() {
    initializePreset(PlanetPreset::EARTH_LIKE, 12345);
}

// Destructor
PlanetTheme::~PlanetTheme() = default;

// Initialize from preset
void PlanetTheme::initializePreset(PlanetPreset preset, uint32_t seed) {
    switch (preset) {
        case PlanetPreset::EARTH_LIKE:
            m_data = createEarthLike(seed);
            break;
        case PlanetPreset::ALIEN_PURPLE:
            m_data = createAlienPurple(seed);
            break;
        case PlanetPreset::ALIEN_RED:
            m_data = createAlienRed(seed);
            break;
        case PlanetPreset::ALIEN_BLUE:
            m_data = createAlienBlue(seed);
            break;
        case PlanetPreset::FROZEN_WORLD:
            m_data = createFrozenWorld(seed);
            break;
        case PlanetPreset::DESERT_WORLD:
            m_data = createDesertWorld(seed);
            break;
        case PlanetPreset::OCEAN_WORLD:
            m_data = createOceanWorld(seed);
            break;
        case PlanetPreset::VOLCANIC_WORLD:
            m_data = createVolcanicWorld(seed);
            break;
        case PlanetPreset::BIOLUMINESCENT:
            m_data = createBioluminescent(seed);
            break;
        case PlanetPreset::CRYSTAL_WORLD:
            m_data = createCrystalWorld(seed);
            break;
        case PlanetPreset::TOXIC_WORLD:
            m_data = createToxicWorld(seed);
            break;
        case PlanetPreset::ANCIENT_WORLD:
            m_data = createAncientWorld(seed);
            break;
        case PlanetPreset::CUSTOM:
        default:
            m_data = createEarthLike(seed);
            break;
    }
    m_data.preset = preset;
    m_data.seed = seed;
}

// Generate random theme
void PlanetTheme::generateRandom(uint32_t seed) {
    std::mt19937 rng(seed);
    generateRandomPalette(rng);
    m_data.preset = PlanetPreset::CUSTOM;
    m_data.seed = seed;
    m_data.name = "Random Planet " + std::to_string(seed);
}

// Set theme data directly
void PlanetTheme::setThemeData(const PlanetThemeData& data) {
    m_data = data;
}

// Set time of day (normalized 0-1)
void PlanetTheme::setTimeOfDay(float normalizedTime) {
    m_currentTime = std::clamp(normalizedTime, 0.0f, 1.0f);
}

// Set time of day (enum)
void PlanetTheme::setTimeOfDay(TimeOfDay time) {
    switch (time) {
        case TimeOfDay::DAWN:      m_currentTime = 0.2f; break;
        case TimeOfDay::MORNING:   m_currentTime = 0.35f; break;
        case TimeOfDay::NOON:      m_currentTime = 0.5f; break;
        case TimeOfDay::AFTERNOON: m_currentTime = 0.65f; break;
        case TimeOfDay::DUSK:      m_currentTime = 0.8f; break;
        case TimeOfDay::NIGHT:     m_currentTime = 0.0f; break;
    }
}

// Get current interpolated atmosphere
AtmosphereSettings PlanetTheme::getCurrentAtmosphere() const {
    // Determine which two atmosphere settings to blend
    if (m_currentTime < 0.25f) {
        // Night to Dawn
        float t = m_currentTime / 0.25f;
        return interpolateAtmosphere(m_data.atmosphereNight, m_data.atmosphereDawn, t);
    } else if (m_currentTime < 0.5f) {
        // Dawn to Noon
        float t = (m_currentTime - 0.25f) / 0.25f;
        return interpolateAtmosphere(m_data.atmosphereDawn, m_data.atmosphereNoon, t);
    } else if (m_currentTime < 0.75f) {
        // Noon to Dusk
        float t = (m_currentTime - 0.5f) / 0.25f;
        return interpolateAtmosphere(m_data.atmosphereNoon, m_data.atmosphereDusk, t);
    } else {
        // Dusk to Night
        float t = (m_currentTime - 0.75f) / 0.25f;
        return interpolateAtmosphere(m_data.atmosphereDusk, m_data.atmosphereNight, t);
    }
}

// Interpolate atmosphere settings
AtmosphereSettings PlanetTheme::interpolateAtmosphere(const AtmosphereSettings& a, const AtmosphereSettings& b, float t) const {
    AtmosphereSettings result;

    result.skyZenithColor = glm::mix(a.skyZenithColor, b.skyZenithColor, t);
    result.skyHorizonColor = glm::mix(a.skyHorizonColor, b.skyHorizonColor, t);
    result.sunColor = glm::mix(a.sunColor, b.sunColor, t);
    result.moonColor = glm::mix(a.moonColor, b.moonColor, t);
    result.fogColor = glm::mix(a.fogColor, b.fogColor, t);
    result.ambientColor = glm::mix(a.ambientColor, b.ambientColor, t);

    result.fogDensity = glm::mix(a.fogDensity, b.fogDensity, t);
    result.fogStart = glm::mix(a.fogStart, b.fogStart, t);
    result.rayleighScattering = glm::mix(a.rayleighScattering, b.rayleighScattering, t);
    result.mieScattering = glm::mix(a.mieScattering, b.mieScattering, t);

    result.sunIntensity = glm::mix(a.sunIntensity, b.sunIntensity, t);
    result.moonIntensity = glm::mix(a.moonIntensity, b.moonIntensity, t);
    result.starVisibility = glm::mix(a.starVisibility, b.starVisibility, t);

    result.hasAurora = (t < 0.5f) ? a.hasAurora : b.hasAurora;
    result.auroraColor1 = glm::mix(a.auroraColor1, b.auroraColor1, t);
    result.auroraColor2 = glm::mix(a.auroraColor2, b.auroraColor2, t);
    result.auroraIntensity = glm::mix(a.auroraIntensity, b.auroraIntensity, t);

    return result;
}

// Color query helpers
glm::vec3 PlanetTheme::getWaterColor(float depth) const {
    float t = std::clamp(depth, 0.0f, 1.0f);
    return glm::mix(m_data.terrain.shallowWaterColor, m_data.terrain.deepWaterColor, t);
}

glm::vec3 PlanetTheme::getSkyColor(float elevation) const {
    auto atm = getCurrentAtmosphere();
    float t = std::clamp(elevation, 0.0f, 1.0f);
    return glm::mix(atm.skyHorizonColor, atm.skyZenithColor, t);
}

glm::vec3 PlanetTheme::getFogColor() const {
    return getCurrentAtmosphere().fogColor;
}

glm::vec3 PlanetTheme::getAmbientColor() const {
    return getCurrentAtmosphere().ambientColor;
}

glm::vec3 PlanetTheme::getSunColor() const {
    return getCurrentAtmosphere().sunColor;
}

// Get terrain color based on parameters
glm::vec3 PlanetTheme::getTerrainColor(float height, float slope, float moisture) const {
    glm::vec3 color;

    // Height-based base color
    if (height < 0.0f) {
        color = m_data.terrain.sandColor;
    } else if (height < 0.3f) {
        float t = height / 0.3f;
        color = glm::mix(m_data.terrain.sandColor, m_data.terrain.grassColor, t * moisture);
    } else if (height < 0.6f) {
        float t = (height - 0.3f) / 0.3f;
        color = glm::mix(m_data.terrain.grassColor, m_data.terrain.rockColor, t);
    } else if (height < 0.85f) {
        color = m_data.terrain.rockColor;
    } else {
        float t = (height - 0.85f) / 0.15f;
        color = glm::mix(m_data.terrain.rockColor, m_data.terrain.snowColor, t);
    }

    // Slope influence - steeper slopes show more rock/cliff
    if (slope > 0.5f) {
        float t = (slope - 0.5f) * 2.0f;
        color = glm::mix(color, m_data.terrain.cliffColor, t);
    }

    return color;
}

glm::vec3 PlanetTheme::blendTerrainColors(const glm::vec3& base, float height) const {
    // Apply altitude-based color adjustment
    float brightnessMod = 1.0f - (height * 0.2f);
    return base * brightnessMod;
}

// Apply theme to biome system - modifies biome colors based on planet theme
void PlanetTheme::applyToBiomeSystem(BiomeSystem& biomeSystem) const {
    // Apply terrain palette colors to appropriate biomes
    biomeSystem.setBaseColor(BiomeType::BEACH_SANDY, m_data.terrain.sandColor);
    biomeSystem.setBaseColor(BiomeType::BEACH_ROCKY, m_data.terrain.rockColor);

    // Apply vegetation colors to forest/grassland biomes
    biomeSystem.setBaseColor(BiomeType::GRASSLAND, m_data.terrain.grassColor);
    biomeSystem.setBaseColor(BiomeType::SAVANNA, m_data.vegetation.grassBaseColor);
    biomeSystem.setBaseColor(BiomeType::TEMPERATE_FOREST, m_data.terrain.forestColor);
    biomeSystem.setBaseColor(BiomeType::TROPICAL_RAINFOREST, m_data.terrain.jungleColor);
    biomeSystem.setBaseColor(BiomeType::BOREAL_FOREST, m_data.terrain.forestColor * 0.8f);
    biomeSystem.setBaseColor(BiomeType::MOUNTAIN_FOREST, m_data.terrain.forestColor * 0.9f);
    biomeSystem.setBaseColor(BiomeType::SHRUBLAND, m_data.terrain.shrubColor);

    // Apply snow/ice colors
    biomeSystem.setBaseColor(BiomeType::TUNDRA, m_data.terrain.snowColor * 0.85f);
    biomeSystem.setBaseColor(BiomeType::GLACIER, m_data.terrain.glacierColor);
    biomeSystem.setBaseColor(BiomeType::DESERT_COLD, m_data.terrain.snowColor * 0.7f + m_data.terrain.dirtColor * 0.3f);

    // Apply desert/rock colors
    biomeSystem.setBaseColor(BiomeType::DESERT_HOT, m_data.terrain.sandColor * 0.9f + m_data.terrain.dirtColor * 0.1f);
    biomeSystem.setBaseColor(BiomeType::ROCKY_HIGHLANDS, m_data.terrain.rockColor);
    biomeSystem.setBaseColor(BiomeType::ALPINE_MEADOW, m_data.terrain.grassColor * 0.7f + m_data.terrain.rockColor * 0.3f);

    // Apply volcanic colors
    biomeSystem.setBaseColor(BiomeType::VOLCANIC, m_data.terrain.ashColor);
    biomeSystem.setBaseColor(BiomeType::LAVA_FIELD, m_data.terrain.lavaColor);

    // Apply special terrain colors
    biomeSystem.setBaseColor(BiomeType::SWAMP, m_data.terrain.mossColor * 0.6f + m_data.terrain.dirtColor * 0.4f);
    biomeSystem.setBaseColor(BiomeType::WETLAND, m_data.terrain.mossColor * 0.5f + m_data.terrain.grassColor * 0.5f);
    biomeSystem.setBaseColor(BiomeType::MANGROVE, m_data.terrain.mossColor * 0.4f + m_data.terrain.dirtColor * 0.6f);

    // Apply water biome accent colors (shore tints)
    biomeSystem.setAccentColor(BiomeType::SHALLOW_WATER, m_data.terrain.shallowWaterColor);
    biomeSystem.setAccentColor(BiomeType::OCEAN, m_data.terrain.deepWaterColor);
    biomeSystem.setAccentColor(BiomeType::DEEP_OCEAN, m_data.terrain.deepWaterColor * 0.7f);
    biomeSystem.setAccentColor(BiomeType::CORAL_REEF, m_data.terrain.shallowWaterColor * 1.1f);
    biomeSystem.setAccentColor(BiomeType::KELP_FOREST, m_data.terrain.deepWaterColor * 0.9f + glm::vec3(0.0f, 0.1f, 0.0f));

    // Apply crystal/special colors if applicable
    if (m_data.preset == PlanetPreset::CRYSTAL_WORLD) {
        biomeSystem.setAccentColor(BiomeType::ROCKY_HIGHLANDS, m_data.terrain.crystalColor);
        biomeSystem.setAccentColor(BiomeType::ALPINE_MEADOW, m_data.terrain.crystalColor * 0.5f);
    }
}

// Get shader constants
PlanetTheme::ShaderConstants PlanetTheme::getShaderConstants() const {
    auto atm = getCurrentAtmosphere();
    ShaderConstants constants;

    constants.skyZenithColor = glm::vec4(atm.skyZenithColor, 1.0f);
    constants.skyHorizonColor = glm::vec4(atm.skyHorizonColor, 1.0f);
    constants.sunColor = glm::vec4(atm.sunColor, 1.0f);
    constants.fogColor = glm::vec4(atm.fogColor, 1.0f);
    constants.ambientColor = glm::vec4(atm.ambientColor, 1.0f);

    constants.waterDeepColor = glm::vec4(m_data.terrain.deepWaterColor, 1.0f);
    constants.waterShallowColor = glm::vec4(m_data.terrain.shallowWaterColor, 1.0f);

    constants.colorFilter = glm::vec4(m_data.colorGrading.colorFilter, 1.0f);
    constants.shadowColor = glm::vec4(m_data.colorGrading.shadowColor, 1.0f);
    constants.highlightColor = glm::vec4(m_data.colorGrading.highlightColor, 1.0f);

    constants.fogDensity = atm.fogDensity;
    constants.fogStart = atm.fogStart;
    constants.sunIntensity = atm.sunIntensity;
    constants.contrast = m_data.colorGrading.contrast;

    constants.saturation = m_data.colorGrading.saturation;
    constants.brightness = m_data.colorGrading.brightness;
    constants.timeOfDay = m_currentTime;
    constants.padding = 0.0f;

    return constants;
}

// HSV to RGB conversion
glm::vec3 PlanetTheme::hsvToRgb(const glm::vec3& hsv) const {
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;

    if (s <= 0.0f) {
        return glm::vec3(v);
    }

    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    h /= 60.0f;

    int i = static_cast<int>(h);
    float f = h - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i) {
        case 0: return glm::vec3(v, t, p);
        case 1: return glm::vec3(q, v, p);
        case 2: return glm::vec3(p, v, t);
        case 3: return glm::vec3(p, q, v);
        case 4: return glm::vec3(t, p, v);
        default: return glm::vec3(v, p, q);
    }
}

// RGB to HSV conversion
glm::vec3 PlanetTheme::rgbToHsv(const glm::vec3& rgb) const {
    float r = rgb.r, g = rgb.g, b = rgb.b;
    float maxVal = std::max({r, g, b});
    float minVal = std::min({r, g, b});
    float delta = maxVal - minVal;

    glm::vec3 hsv;
    hsv.z = maxVal; // Value

    if (maxVal > 0.0f) {
        hsv.y = delta / maxVal; // Saturation
    } else {
        hsv.y = 0.0f;
        hsv.x = 0.0f;
        return hsv;
    }

    if (delta < 0.00001f) {
        hsv.x = 0.0f;
        return hsv;
    }

    // Hue calculation
    if (r >= maxVal) {
        hsv.x = (g - b) / delta;
    } else if (g >= maxVal) {
        hsv.x = 2.0f + (b - r) / delta;
    } else {
        hsv.x = 4.0f + (r - g) / delta;
    }

    hsv.x *= 60.0f;
    if (hsv.x < 0.0f) hsv.x += 360.0f;

    return hsv;
}

// Shift hue of a color
glm::vec3 PlanetTheme::shiftHue(const glm::vec3& color, float shift) const {
    glm::vec3 hsv = rgbToHsv(color);
    hsv.x = std::fmod(hsv.x + shift, 360.0f);
    if (hsv.x < 0.0f) hsv.x += 360.0f;
    return hsvToRgb(hsv);
}

// Adjust saturation
glm::vec3 PlanetTheme::adjustSaturation(const glm::vec3& color, float factor) const {
    glm::vec3 hsv = rgbToHsv(color);
    hsv.y = std::clamp(hsv.y * factor, 0.0f, 1.0f);
    return hsvToRgb(hsv);
}

// Adjust brightness
glm::vec3 PlanetTheme::adjustBrightness(const glm::vec3& color, float factor) const {
    glm::vec3 hsv = rgbToHsv(color);
    hsv.z = std::clamp(hsv.z * factor, 0.0f, 1.0f);
    return hsvToRgb(hsv);
}

// Generate harmonious color
glm::vec3 PlanetTheme::generateHarmoniousColor(std::mt19937& rng, const glm::vec3& baseColor, float hueRange) const {
    std::uniform_real_distribution<float> dist(-hueRange, hueRange);
    return shiftHue(baseColor, dist(rng));
}

// Generate complementary color
glm::vec3 PlanetTheme::generateComplementaryColor(const glm::vec3& color) const {
    return shiftHue(color, 180.0f);
}

// Generate analogous color
glm::vec3 PlanetTheme::generateAnalogousColor(const glm::vec3& color, float offset) const {
    return shiftHue(color, offset);
}

// Generate random palette
void PlanetTheme::generateRandomPalette(std::mt19937& rng) {
    std::uniform_real_distribution<float> hueDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> satDist(0.4f, 1.0f);
    std::uniform_real_distribution<float> valDist(0.3f, 0.9f);
    std::uniform_real_distribution<float> smallVar(-30.0f, 30.0f);

    // Generate base hue for the planet
    float baseHue = hueDist(rng);
    float baseSat = satDist(rng);
    float baseVal = valDist(rng);

    glm::vec3 baseColor = hsvToRgb(glm::vec3(baseHue, baseSat, baseVal));
    glm::vec3 complementary = generateComplementaryColor(baseColor);
    glm::vec3 analogous1 = generateAnalogousColor(baseColor, 30.0f);
    glm::vec3 analogous2 = generateAnalogousColor(baseColor, -30.0f);

    // Atmosphere
    m_data.atmosphere.skyZenithColor = adjustBrightness(baseColor, 0.8f);
    m_data.atmosphere.skyHorizonColor = adjustSaturation(analogous1, 0.6f);
    m_data.atmosphere.sunColor = glm::vec3(1.0f, 0.95f, 0.8f);
    m_data.atmosphere.moonColor = glm::vec3(0.8f, 0.85f, 1.0f);
    m_data.atmosphere.fogColor = adjustSaturation(m_data.atmosphere.skyHorizonColor, 0.3f);
    m_data.atmosphere.ambientColor = adjustBrightness(baseColor, 0.3f);
    m_data.atmosphere.fogDensity = 0.02f;
    m_data.atmosphere.fogStart = 50.0f;
    m_data.atmosphere.rayleighScattering = 0.0025f;
    m_data.atmosphere.mieScattering = 0.001f;
    m_data.atmosphere.sunIntensity = 1.0f;
    m_data.atmosphere.moonIntensity = 0.2f;
    m_data.atmosphere.starVisibility = 0.8f;
    m_data.atmosphere.hasAurora = (rng() % 4 == 0);
    m_data.atmosphere.auroraColor1 = generateAnalogousColor(baseColor, 60.0f);
    m_data.atmosphere.auroraColor2 = generateAnalogousColor(baseColor, -60.0f);
    m_data.atmosphere.auroraIntensity = 0.5f;

    // Copy to time variants
    m_data.atmosphereNoon = m_data.atmosphere;

    m_data.atmosphereDawn = m_data.atmosphere;
    m_data.atmosphereDawn.skyHorizonColor = shiftHue(m_data.atmosphere.skyHorizonColor, 20.0f);
    m_data.atmosphereDawn.sunIntensity = 0.6f;

    m_data.atmosphereDusk = m_data.atmosphere;
    m_data.atmosphereDusk.skyHorizonColor = shiftHue(m_data.atmosphere.skyHorizonColor, -20.0f);
    m_data.atmosphereDusk.sunIntensity = 0.5f;

    m_data.atmosphereNight = m_data.atmosphere;
    m_data.atmosphereNight.skyZenithColor = adjustBrightness(m_data.atmosphere.skyZenithColor, 0.1f);
    m_data.atmosphereNight.skyHorizonColor = adjustBrightness(m_data.atmosphere.skyHorizonColor, 0.15f);
    m_data.atmosphereNight.sunIntensity = 0.0f;
    m_data.atmosphereNight.moonIntensity = 0.3f;
    m_data.atmosphereNight.starVisibility = 1.0f;

    // Terrain
    m_data.terrain.deepWaterColor = adjustBrightness(complementary, 0.4f);
    m_data.terrain.shallowWaterColor = adjustBrightness(complementary, 0.7f);
    m_data.terrain.waterFoamColor = glm::vec3(0.9f);
    m_data.terrain.waterReflectionTint = glm::vec3(1.0f);
    m_data.terrain.sandColor = hsvToRgb(glm::vec3(hueDist(rng), 0.3f, 0.8f));
    m_data.terrain.dirtColor = adjustBrightness(m_data.terrain.sandColor, 0.6f);
    m_data.terrain.rockColor = hsvToRgb(glm::vec3(hueDist(rng), 0.2f, 0.5f));
    m_data.terrain.cliffColor = adjustBrightness(m_data.terrain.rockColor, 0.7f);
    m_data.terrain.grassColor = analogous2;
    m_data.terrain.forestColor = adjustBrightness(analogous2, 0.7f);
    m_data.terrain.jungleColor = adjustSaturation(analogous2, 1.2f);
    m_data.terrain.shrubColor = generateAnalogousColor(analogous2, 15.0f);
    m_data.terrain.snowColor = glm::vec3(0.95f, 0.97f, 1.0f);
    m_data.terrain.iceColor = glm::vec3(0.8f, 0.9f, 1.0f);
    m_data.terrain.glacierColor = glm::vec3(0.7f, 0.85f, 0.95f);
    m_data.terrain.lavaColor = glm::vec3(1.0f, 0.3f, 0.0f);
    m_data.terrain.ashColor = glm::vec3(0.3f, 0.3f, 0.35f);
    m_data.terrain.crystalColor = hsvToRgb(glm::vec3(hueDist(rng), 0.8f, 0.9f));
    m_data.terrain.mossColor = generateAnalogousColor(analogous2, -20.0f);

    // Vegetation
    m_data.vegetation.treeLeafColor = analogous2;
    m_data.vegetation.treeBarkColor = adjustSaturation(m_data.terrain.dirtColor, 0.5f);
    m_data.vegetation.treeLeafVariation = glm::vec3(0.1f);
    m_data.vegetation.grassBaseColor = adjustBrightness(analogous2, 0.8f);
    m_data.vegetation.grassTipColor = adjustBrightness(analogous2, 1.1f);
    m_data.vegetation.grassVariation = glm::vec3(0.05f);

    for (int i = 0; i < 6; i++) {
        m_data.vegetation.flowerColors[i] = hsvToRgb(glm::vec3(hueDist(rng), satDist(rng), valDist(rng)));
    }
    for (int i = 0; i < 4; i++) {
        m_data.vegetation.mushroomColors[i] = hsvToRgb(glm::vec3(hueDist(rng), satDist(rng) * 0.5f, valDist(rng)));
    }
    m_data.vegetation.colorSaturation = 1.0f;
    m_data.vegetation.colorBrightness = 1.0f;

    // Creatures
    m_data.creatures.herbivoreBaseTint = adjustSaturation(analogous2, 0.6f);
    m_data.creatures.carnivoreTint = adjustSaturation(complementary, 0.7f);
    m_data.creatures.aquaticTint = m_data.terrain.shallowWaterColor;
    m_data.creatures.flyingTint = adjustBrightness(baseColor, 1.1f);
    m_data.creatures.environmentInfluence = 0.3f;
    m_data.creatures.geneticVariation = 0.2f;

    // Color grading
    m_data.colorGrading.colorFilter = glm::vec3(1.0f);
    m_data.colorGrading.shadowColor = adjustBrightness(complementary, 0.3f);
    m_data.colorGrading.highlightColor = glm::vec3(1.0f);
    m_data.colorGrading.contrast = 1.0f;
    m_data.colorGrading.saturation = 1.0f;
    m_data.colorGrading.brightness = 1.0f;
    m_data.colorGrading.gamma = 1.0f;
    m_data.colorGrading.splitToneShadows = adjustBrightness(complementary, 0.5f);
    m_data.colorGrading.splitToneHighlights = adjustBrightness(baseColor, 1.2f);
    m_data.colorGrading.splitToneBalance = 0.0f;
    m_data.colorGrading.vignetteIntensity = 0.2f;
    m_data.colorGrading.vignetteRadius = 0.8f;
}

// Serialization
void PlanetTheme::serialize(std::vector<uint8_t>& data) const {
    size_t size = sizeof(PlanetThemeData);
    data.resize(size);
    std::memcpy(data.data(), &m_data, size);
}

void PlanetTheme::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() >= sizeof(PlanetThemeData)) {
        std::memcpy(&m_data, data.data(), sizeof(PlanetThemeData));
    }
}

// Utility functions
const char* presetToString(PlanetPreset preset) {
    switch (preset) {
        case PlanetPreset::EARTH_LIKE: return "Earth-Like";
        case PlanetPreset::ALIEN_PURPLE: return "Alien Purple";
        case PlanetPreset::ALIEN_RED: return "Alien Red";
        case PlanetPreset::ALIEN_BLUE: return "Alien Blue";
        case PlanetPreset::FROZEN_WORLD: return "Frozen World";
        case PlanetPreset::DESERT_WORLD: return "Desert World";
        case PlanetPreset::OCEAN_WORLD: return "Ocean World";
        case PlanetPreset::VOLCANIC_WORLD: return "Volcanic World";
        case PlanetPreset::BIOLUMINESCENT: return "Bioluminescent";
        case PlanetPreset::CRYSTAL_WORLD: return "Crystal World";
        case PlanetPreset::TOXIC_WORLD: return "Toxic World";
        case PlanetPreset::ANCIENT_WORLD: return "Ancient World";
        case PlanetPreset::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

PlanetPreset stringToPreset(const std::string& name) {
    if (name == "Earth-Like") return PlanetPreset::EARTH_LIKE;
    if (name == "Alien Purple") return PlanetPreset::ALIEN_PURPLE;
    if (name == "Alien Red") return PlanetPreset::ALIEN_RED;
    if (name == "Alien Blue") return PlanetPreset::ALIEN_BLUE;
    if (name == "Frozen World") return PlanetPreset::FROZEN_WORLD;
    if (name == "Desert World") return PlanetPreset::DESERT_WORLD;
    if (name == "Ocean World") return PlanetPreset::OCEAN_WORLD;
    if (name == "Volcanic World") return PlanetPreset::VOLCANIC_WORLD;
    if (name == "Bioluminescent") return PlanetPreset::BIOLUMINESCENT;
    if (name == "Crystal World") return PlanetPreset::CRYSTAL_WORLD;
    if (name == "Toxic World") return PlanetPreset::TOXIC_WORLD;
    if (name == "Ancient World") return PlanetPreset::ANCIENT_WORLD;
    return PlanetPreset::CUSTOM;
}

glm::vec3 temperatureToColor(float temperature) {
    // Temperature in Kelvin-like scale, maps to black body radiation colors
    float t = std::clamp(temperature, 0.0f, 1.0f);

    if (t < 0.25f) {
        // Cold: blue to cyan
        float s = t / 0.25f;
        return glm::mix(glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.5f, 1.0f), s);
    } else if (t < 0.5f) {
        // Cool to warm: cyan to green to yellow
        float s = (t - 0.25f) / 0.25f;
        return glm::mix(glm::vec3(0.0f, 0.5f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f), s);
    } else if (t < 0.75f) {
        // Warm: yellow to orange
        float s = (t - 0.5f) / 0.25f;
        return glm::mix(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.5f, 0.0f), s);
    } else {
        // Hot: orange to red
        float s = (t - 0.75f) / 0.25f;
        return glm::mix(glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), s);
    }
}

// ============================================================================
// PRESET GENERATORS
// ============================================================================

// Helper to set up time-of-day atmosphere variants
static void setupTimeVariants(PlanetThemeData& data) {
    data.atmosphereNoon = data.atmosphere;

    // Dawn - warmer horizon, softer light
    data.atmosphereDawn = data.atmosphere;
    data.atmosphereDawn.skyHorizonColor = glm::vec3(1.0f, 0.6f, 0.4f);
    data.atmosphereDawn.sunIntensity = 0.6f;
    data.atmosphereDawn.ambientColor = data.atmosphere.ambientColor * 0.7f;

    // Dusk - orange/red horizon
    data.atmosphereDusk = data.atmosphere;
    data.atmosphereDusk.skyHorizonColor = glm::vec3(1.0f, 0.4f, 0.2f);
    data.atmosphereDusk.sunIntensity = 0.5f;
    data.atmosphereDusk.ambientColor = data.atmosphere.ambientColor * 0.6f;

    // Night - dark sky, stars visible
    data.atmosphereNight = data.atmosphere;
    data.atmosphereNight.skyZenithColor = data.atmosphere.skyZenithColor * 0.05f;
    data.atmosphereNight.skyHorizonColor = data.atmosphere.skyHorizonColor * 0.1f;
    data.atmosphereNight.sunIntensity = 0.0f;
    data.atmosphereNight.moonIntensity = 0.3f;
    data.atmosphereNight.starVisibility = 1.0f;
    data.atmosphereNight.ambientColor = data.atmosphere.ambientColor * 0.15f;
}

// Helper to set default color grading
static void setupDefaultColorGrading(ColorGrading& cg) {
    cg.colorFilter = glm::vec3(1.0f);
    cg.shadowColor = glm::vec3(0.1f, 0.1f, 0.15f);
    cg.highlightColor = glm::vec3(1.0f);
    cg.contrast = 1.0f;
    cg.saturation = 1.0f;
    cg.brightness = 1.0f;
    cg.gamma = 1.0f;
    cg.splitToneShadows = glm::vec3(0.1f, 0.1f, 0.2f);
    cg.splitToneHighlights = glm::vec3(1.0f, 0.98f, 0.95f);
    cg.splitToneBalance = 0.0f;
    cg.vignetteIntensity = 0.15f;
    cg.vignetteRadius = 0.85f;
}

PlanetThemeData PlanetTheme::createEarthLike(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Earth-Like";
    data.preset = PlanetPreset::EARTH_LIKE;
    data.seed = seed;

    // Atmosphere - classic blue sky
    data.atmosphere.skyZenithColor = glm::vec3(0.2f, 0.4f, 0.8f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.6f, 0.75f, 0.9f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.98f, 0.9f);
    data.atmosphere.moonColor = glm::vec3(0.8f, 0.85f, 0.9f);
    data.atmosphere.fogColor = glm::vec3(0.7f, 0.8f, 0.9f);
    data.atmosphere.ambientColor = glm::vec3(0.4f, 0.45f, 0.5f);
    data.atmosphere.fogDensity = 0.015f;
    data.atmosphere.fogStart = 100.0f;
    data.atmosphere.rayleighScattering = 0.0025f;
    data.atmosphere.mieScattering = 0.001f;
    data.atmosphere.sunIntensity = 1.0f;
    data.atmosphere.moonIntensity = 0.15f;
    data.atmosphere.starVisibility = 0.7f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.2f, 0.8f, 0.3f);
    data.atmosphere.auroraColor2 = glm::vec3(0.3f, 0.4f, 0.9f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - Earth colors
    data.terrain.deepWaterColor = glm::vec3(0.0f, 0.1f, 0.3f);
    data.terrain.shallowWaterColor = glm::vec3(0.1f, 0.4f, 0.5f);
    data.terrain.waterFoamColor = glm::vec3(0.9f, 0.95f, 1.0f);
    data.terrain.waterReflectionTint = glm::vec3(1.0f);
    data.terrain.sandColor = glm::vec3(0.76f, 0.7f, 0.5f);
    data.terrain.dirtColor = glm::vec3(0.4f, 0.3f, 0.2f);
    data.terrain.rockColor = glm::vec3(0.5f, 0.5f, 0.5f);
    data.terrain.cliffColor = glm::vec3(0.6f, 0.55f, 0.5f);
    data.terrain.grassColor = glm::vec3(0.2f, 0.5f, 0.15f);
    data.terrain.forestColor = glm::vec3(0.1f, 0.35f, 0.1f);
    data.terrain.jungleColor = glm::vec3(0.05f, 0.4f, 0.1f);
    data.terrain.shrubColor = glm::vec3(0.3f, 0.45f, 0.2f);
    data.terrain.snowColor = glm::vec3(0.95f, 0.97f, 1.0f);
    data.terrain.iceColor = glm::vec3(0.8f, 0.9f, 1.0f);
    data.terrain.glacierColor = glm::vec3(0.7f, 0.85f, 0.95f);
    data.terrain.lavaColor = glm::vec3(1.0f, 0.3f, 0.0f);
    data.terrain.ashColor = glm::vec3(0.3f, 0.3f, 0.32f);
    data.terrain.crystalColor = glm::vec3(0.8f, 0.85f, 0.9f);
    data.terrain.mossColor = glm::vec3(0.2f, 0.4f, 0.15f);

    // Vegetation
    data.vegetation.treeLeafColor = glm::vec3(0.15f, 0.45f, 0.1f);
    data.vegetation.treeBarkColor = glm::vec3(0.3f, 0.2f, 0.1f);
    data.vegetation.treeLeafVariation = glm::vec3(0.1f, 0.15f, 0.05f);
    data.vegetation.grassBaseColor = glm::vec3(0.15f, 0.4f, 0.1f);
    data.vegetation.grassTipColor = glm::vec3(0.3f, 0.55f, 0.2f);
    data.vegetation.grassVariation = glm::vec3(0.05f);
    data.vegetation.flowerColors[0] = glm::vec3(1.0f, 0.2f, 0.2f);
    data.vegetation.flowerColors[1] = glm::vec3(1.0f, 1.0f, 0.2f);
    data.vegetation.flowerColors[2] = glm::vec3(0.9f, 0.4f, 0.8f);
    data.vegetation.flowerColors[3] = glm::vec3(0.3f, 0.3f, 0.9f);
    data.vegetation.flowerColors[4] = glm::vec3(1.0f, 0.6f, 0.2f);
    data.vegetation.flowerColors[5] = glm::vec3(1.0f, 1.0f, 1.0f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.8f, 0.7f, 0.6f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.9f, 0.2f, 0.1f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.6f, 0.5f, 0.4f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.9f, 0.85f, 0.7f);
    data.vegetation.colorSaturation = 1.0f;
    data.vegetation.colorBrightness = 1.0f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.6f, 0.5f, 0.4f);
    data.creatures.carnivoreTint = glm::vec3(0.5f, 0.4f, 0.35f);
    data.creatures.aquaticTint = glm::vec3(0.3f, 0.5f, 0.6f);
    data.creatures.flyingTint = glm::vec3(0.5f, 0.5f, 0.55f);
    data.creatures.environmentInfluence = 0.2f;
    data.creatures.geneticVariation = 0.25f;

    setupDefaultColorGrading(data.colorGrading);
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createAlienPurple(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Alien Purple";
    data.preset = PlanetPreset::ALIEN_PURPLE;
    data.seed = seed;

    // Atmosphere - orange/pink sky
    data.atmosphere.skyZenithColor = glm::vec3(0.6f, 0.3f, 0.5f);
    data.atmosphere.skyHorizonColor = glm::vec3(1.0f, 0.6f, 0.4f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.8f, 0.6f);
    data.atmosphere.moonColor = glm::vec3(0.7f, 0.6f, 0.9f);
    data.atmosphere.fogColor = glm::vec3(0.8f, 0.5f, 0.6f);
    data.atmosphere.ambientColor = glm::vec3(0.5f, 0.35f, 0.45f);
    data.atmosphere.fogDensity = 0.02f;
    data.atmosphere.fogStart = 80.0f;
    data.atmosphere.rayleighScattering = 0.003f;
    data.atmosphere.mieScattering = 0.0015f;
    data.atmosphere.sunIntensity = 0.9f;
    data.atmosphere.moonIntensity = 0.2f;
    data.atmosphere.starVisibility = 0.6f;
    data.atmosphere.hasAurora = true;
    data.atmosphere.auroraColor1 = glm::vec3(0.9f, 0.3f, 0.6f);
    data.atmosphere.auroraColor2 = glm::vec3(0.5f, 0.2f, 0.8f);
    data.atmosphere.auroraIntensity = 0.4f;

    // Terrain - purple/pink vegetation
    data.terrain.deepWaterColor = glm::vec3(0.2f, 0.1f, 0.3f);
    data.terrain.shallowWaterColor = glm::vec3(0.4f, 0.25f, 0.5f);
    data.terrain.waterFoamColor = glm::vec3(0.9f, 0.7f, 0.9f);
    data.terrain.waterReflectionTint = glm::vec3(1.0f, 0.9f, 1.0f);
    data.terrain.sandColor = glm::vec3(0.7f, 0.55f, 0.5f);
    data.terrain.dirtColor = glm::vec3(0.4f, 0.25f, 0.3f);
    data.terrain.rockColor = glm::vec3(0.45f, 0.35f, 0.45f);
    data.terrain.cliffColor = glm::vec3(0.55f, 0.4f, 0.5f);
    data.terrain.grassColor = glm::vec3(0.6f, 0.2f, 0.5f);
    data.terrain.forestColor = glm::vec3(0.4f, 0.15f, 0.4f);
    data.terrain.jungleColor = glm::vec3(0.5f, 0.1f, 0.45f);
    data.terrain.shrubColor = glm::vec3(0.7f, 0.3f, 0.55f);
    data.terrain.snowColor = glm::vec3(0.95f, 0.9f, 0.95f);
    data.terrain.iceColor = glm::vec3(0.85f, 0.8f, 0.9f);
    data.terrain.glacierColor = glm::vec3(0.75f, 0.7f, 0.85f);
    data.terrain.lavaColor = glm::vec3(0.9f, 0.2f, 0.5f);
    data.terrain.ashColor = glm::vec3(0.35f, 0.3f, 0.35f);
    data.terrain.crystalColor = glm::vec3(0.8f, 0.4f, 0.9f);
    data.terrain.mossColor = glm::vec3(0.5f, 0.25f, 0.45f);

    // Vegetation - purple/pink tones
    data.vegetation.treeLeafColor = glm::vec3(0.55f, 0.2f, 0.5f);
    data.vegetation.treeBarkColor = glm::vec3(0.35f, 0.2f, 0.25f);
    data.vegetation.treeLeafVariation = glm::vec3(0.15f, 0.1f, 0.1f);
    data.vegetation.grassBaseColor = glm::vec3(0.5f, 0.15f, 0.45f);
    data.vegetation.grassTipColor = glm::vec3(0.7f, 0.35f, 0.6f);
    data.vegetation.grassVariation = glm::vec3(0.08f);
    data.vegetation.flowerColors[0] = glm::vec3(1.0f, 0.5f, 0.3f);
    data.vegetation.flowerColors[1] = glm::vec3(0.9f, 0.3f, 0.7f);
    data.vegetation.flowerColors[2] = glm::vec3(0.6f, 0.9f, 0.3f);
    data.vegetation.flowerColors[3] = glm::vec3(0.3f, 0.8f, 0.9f);
    data.vegetation.flowerColors[4] = glm::vec3(1.0f, 0.8f, 0.2f);
    data.vegetation.flowerColors[5] = glm::vec3(0.9f, 0.9f, 0.5f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.7f, 0.5f, 0.7f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.9f, 0.4f, 0.6f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.5f, 0.3f, 0.5f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.8f, 0.6f, 0.8f);
    data.vegetation.colorSaturation = 1.1f;
    data.vegetation.colorBrightness = 1.0f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.6f, 0.4f, 0.55f);
    data.creatures.carnivoreTint = glm::vec3(0.5f, 0.3f, 0.45f);
    data.creatures.aquaticTint = glm::vec3(0.45f, 0.3f, 0.55f);
    data.creatures.flyingTint = glm::vec3(0.7f, 0.5f, 0.65f);
    data.creatures.environmentInfluence = 0.3f;
    data.creatures.geneticVariation = 0.2f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(1.0f, 0.95f, 1.0f);
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createAlienRed(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Alien Red";
    data.preset = PlanetPreset::ALIEN_RED;
    data.seed = seed;

    // Atmosphere - yellow/amber sky
    data.atmosphere.skyZenithColor = glm::vec3(0.7f, 0.5f, 0.2f);
    data.atmosphere.skyHorizonColor = glm::vec3(1.0f, 0.8f, 0.4f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.9f, 0.7f);
    data.atmosphere.moonColor = glm::vec3(0.9f, 0.7f, 0.5f);
    data.atmosphere.fogColor = glm::vec3(0.8f, 0.6f, 0.4f);
    data.atmosphere.ambientColor = glm::vec3(0.5f, 0.4f, 0.3f);
    data.atmosphere.fogDensity = 0.025f;
    data.atmosphere.fogStart = 70.0f;
    data.atmosphere.rayleighScattering = 0.002f;
    data.atmosphere.mieScattering = 0.002f;
    data.atmosphere.sunIntensity = 1.1f;
    data.atmosphere.moonIntensity = 0.15f;
    data.atmosphere.starVisibility = 0.5f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.9f, 0.5f, 0.2f);
    data.atmosphere.auroraColor2 = glm::vec3(0.8f, 0.3f, 0.1f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - red vegetation
    data.terrain.deepWaterColor = glm::vec3(0.3f, 0.15f, 0.1f);
    data.terrain.shallowWaterColor = glm::vec3(0.5f, 0.3f, 0.2f);
    data.terrain.waterFoamColor = glm::vec3(0.95f, 0.85f, 0.75f);
    data.terrain.waterReflectionTint = glm::vec3(1.0f, 0.95f, 0.9f);
    data.terrain.sandColor = glm::vec3(0.8f, 0.6f, 0.4f);
    data.terrain.dirtColor = glm::vec3(0.5f, 0.3f, 0.2f);
    data.terrain.rockColor = glm::vec3(0.55f, 0.4f, 0.35f);
    data.terrain.cliffColor = glm::vec3(0.6f, 0.45f, 0.4f);
    data.terrain.grassColor = glm::vec3(0.7f, 0.2f, 0.15f);
    data.terrain.forestColor = glm::vec3(0.5f, 0.15f, 0.1f);
    data.terrain.jungleColor = glm::vec3(0.6f, 0.1f, 0.1f);
    data.terrain.shrubColor = glm::vec3(0.8f, 0.3f, 0.2f);
    data.terrain.snowColor = glm::vec3(0.95f, 0.92f, 0.88f);
    data.terrain.iceColor = glm::vec3(0.9f, 0.85f, 0.8f);
    data.terrain.glacierColor = glm::vec3(0.85f, 0.75f, 0.7f);
    data.terrain.lavaColor = glm::vec3(1.0f, 0.4f, 0.0f);
    data.terrain.ashColor = glm::vec3(0.4f, 0.35f, 0.3f);
    data.terrain.crystalColor = glm::vec3(0.9f, 0.5f, 0.3f);
    data.terrain.mossColor = glm::vec3(0.6f, 0.25f, 0.15f);

    // Vegetation - red/orange tones
    data.vegetation.treeLeafColor = glm::vec3(0.65f, 0.2f, 0.1f);
    data.vegetation.treeBarkColor = glm::vec3(0.4f, 0.25f, 0.15f);
    data.vegetation.treeLeafVariation = glm::vec3(0.15f, 0.1f, 0.05f);
    data.vegetation.grassBaseColor = glm::vec3(0.6f, 0.15f, 0.1f);
    data.vegetation.grassTipColor = glm::vec3(0.85f, 0.35f, 0.2f);
    data.vegetation.grassVariation = glm::vec3(0.1f, 0.05f, 0.03f);
    data.vegetation.flowerColors[0] = glm::vec3(1.0f, 0.8f, 0.2f);
    data.vegetation.flowerColors[1] = glm::vec3(0.9f, 0.5f, 0.9f);
    data.vegetation.flowerColors[2] = glm::vec3(0.3f, 0.7f, 0.9f);
    data.vegetation.flowerColors[3] = glm::vec3(0.4f, 0.9f, 0.4f);
    data.vegetation.flowerColors[4] = glm::vec3(1.0f, 1.0f, 0.5f);
    data.vegetation.flowerColors[5] = glm::vec3(0.9f, 0.6f, 0.3f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.7f, 0.5f, 0.4f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.8f, 0.3f, 0.2f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.6f, 0.4f, 0.3f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.9f, 0.7f, 0.5f);
    data.vegetation.colorSaturation = 1.15f;
    data.vegetation.colorBrightness = 1.05f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.65f, 0.45f, 0.35f);
    data.creatures.carnivoreTint = glm::vec3(0.55f, 0.35f, 0.25f);
    data.creatures.aquaticTint = glm::vec3(0.5f, 0.35f, 0.3f);
    data.creatures.flyingTint = glm::vec3(0.7f, 0.5f, 0.4f);
    data.creatures.environmentInfluence = 0.35f;
    data.creatures.geneticVariation = 0.2f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(1.0f, 0.95f, 0.9f);
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createAlienBlue(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Alien Blue";
    data.preset = PlanetPreset::ALIEN_BLUE;
    data.seed = seed;

    // Atmosphere - green/teal sky
    data.atmosphere.skyZenithColor = glm::vec3(0.2f, 0.5f, 0.4f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.4f, 0.8f, 0.6f);
    data.atmosphere.sunColor = glm::vec3(0.9f, 1.0f, 0.95f);
    data.atmosphere.moonColor = glm::vec3(0.6f, 0.8f, 0.9f);
    data.atmosphere.fogColor = glm::vec3(0.4f, 0.7f, 0.6f);
    data.atmosphere.ambientColor = glm::vec3(0.35f, 0.5f, 0.45f);
    data.atmosphere.fogDensity = 0.018f;
    data.atmosphere.fogStart = 90.0f;
    data.atmosphere.rayleighScattering = 0.003f;
    data.atmosphere.mieScattering = 0.001f;
    data.atmosphere.sunIntensity = 0.95f;
    data.atmosphere.moonIntensity = 0.25f;
    data.atmosphere.starVisibility = 0.7f;
    data.atmosphere.hasAurora = true;
    data.atmosphere.auroraColor1 = glm::vec3(0.3f, 0.9f, 0.5f);
    data.atmosphere.auroraColor2 = glm::vec3(0.2f, 0.6f, 0.9f);
    data.atmosphere.auroraIntensity = 0.35f;

    // Terrain - blue vegetation
    data.terrain.deepWaterColor = glm::vec3(0.1f, 0.2f, 0.35f);
    data.terrain.shallowWaterColor = glm::vec3(0.2f, 0.45f, 0.55f);
    data.terrain.waterFoamColor = glm::vec3(0.8f, 0.95f, 1.0f);
    data.terrain.waterReflectionTint = glm::vec3(0.9f, 1.0f, 1.0f);
    data.terrain.sandColor = glm::vec3(0.6f, 0.65f, 0.55f);
    data.terrain.dirtColor = glm::vec3(0.35f, 0.35f, 0.3f);
    data.terrain.rockColor = glm::vec3(0.4f, 0.45f, 0.5f);
    data.terrain.cliffColor = glm::vec3(0.45f, 0.5f, 0.55f);
    data.terrain.grassColor = glm::vec3(0.15f, 0.4f, 0.55f);
    data.terrain.forestColor = glm::vec3(0.1f, 0.3f, 0.45f);
    data.terrain.jungleColor = glm::vec3(0.1f, 0.35f, 0.5f);
    data.terrain.shrubColor = glm::vec3(0.2f, 0.5f, 0.6f);
    data.terrain.snowColor = glm::vec3(0.9f, 0.95f, 1.0f);
    data.terrain.iceColor = glm::vec3(0.8f, 0.9f, 1.0f);
    data.terrain.glacierColor = glm::vec3(0.7f, 0.85f, 0.95f);
    data.terrain.lavaColor = glm::vec3(0.3f, 0.8f, 1.0f);
    data.terrain.ashColor = glm::vec3(0.3f, 0.35f, 0.4f);
    data.terrain.crystalColor = glm::vec3(0.4f, 0.7f, 0.9f);
    data.terrain.mossColor = glm::vec3(0.15f, 0.35f, 0.4f);

    // Vegetation - blue/cyan tones
    data.vegetation.treeLeafColor = glm::vec3(0.15f, 0.4f, 0.5f);
    data.vegetation.treeBarkColor = glm::vec3(0.25f, 0.25f, 0.3f);
    data.vegetation.treeLeafVariation = glm::vec3(0.08f, 0.12f, 0.1f);
    data.vegetation.grassBaseColor = glm::vec3(0.1f, 0.35f, 0.5f);
    data.vegetation.grassTipColor = glm::vec3(0.25f, 0.55f, 0.65f);
    data.vegetation.grassVariation = glm::vec3(0.05f, 0.08f, 0.06f);
    data.vegetation.flowerColors[0] = glm::vec3(0.9f, 0.4f, 0.3f);
    data.vegetation.flowerColors[1] = glm::vec3(1.0f, 0.7f, 0.2f);
    data.vegetation.flowerColors[2] = glm::vec3(0.9f, 0.3f, 0.7f);
    data.vegetation.flowerColors[3] = glm::vec3(1.0f, 1.0f, 0.4f);
    data.vegetation.flowerColors[4] = glm::vec3(0.5f, 0.9f, 0.9f);
    data.vegetation.flowerColors[5] = glm::vec3(0.95f, 0.95f, 0.9f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.5f, 0.6f, 0.7f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.3f, 0.5f, 0.7f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.4f, 0.5f, 0.55f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.6f, 0.75f, 0.85f);
    data.vegetation.colorSaturation = 1.05f;
    data.vegetation.colorBrightness = 1.0f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.4f, 0.55f, 0.6f);
    data.creatures.carnivoreTint = glm::vec3(0.35f, 0.45f, 0.5f);
    data.creatures.aquaticTint = glm::vec3(0.3f, 0.5f, 0.6f);
    data.creatures.flyingTint = glm::vec3(0.5f, 0.65f, 0.7f);
    data.creatures.environmentInfluence = 0.3f;
    data.creatures.geneticVariation = 0.22f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.95f, 1.0f, 1.0f);
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createFrozenWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Frozen World";
    data.preset = PlanetPreset::FROZEN_WORLD;
    data.seed = seed;

    // Atmosphere - pale blue, cold
    data.atmosphere.skyZenithColor = glm::vec3(0.4f, 0.5f, 0.7f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.7f, 0.8f, 0.9f);
    data.atmosphere.sunColor = glm::vec3(0.95f, 0.97f, 1.0f);
    data.atmosphere.moonColor = glm::vec3(0.85f, 0.9f, 1.0f);
    data.atmosphere.fogColor = glm::vec3(0.8f, 0.85f, 0.95f);
    data.atmosphere.ambientColor = glm::vec3(0.5f, 0.55f, 0.65f);
    data.atmosphere.fogDensity = 0.03f;
    data.atmosphere.fogStart = 60.0f;
    data.atmosphere.rayleighScattering = 0.004f;
    data.atmosphere.mieScattering = 0.002f;
    data.atmosphere.sunIntensity = 0.7f;
    data.atmosphere.moonIntensity = 0.35f;
    data.atmosphere.starVisibility = 0.9f;
    data.atmosphere.hasAurora = true;
    data.atmosphere.auroraColor1 = glm::vec3(0.2f, 0.9f, 0.4f);
    data.atmosphere.auroraColor2 = glm::vec3(0.3f, 0.5f, 0.95f);
    data.atmosphere.auroraIntensity = 0.6f;

    // Terrain - ice and snow dominant
    data.terrain.deepWaterColor = glm::vec3(0.1f, 0.15f, 0.25f);
    data.terrain.shallowWaterColor = glm::vec3(0.3f, 0.45f, 0.55f);
    data.terrain.waterFoamColor = glm::vec3(0.95f, 0.98f, 1.0f);
    data.terrain.waterReflectionTint = glm::vec3(0.9f, 0.95f, 1.0f);
    data.terrain.sandColor = glm::vec3(0.7f, 0.75f, 0.8f);
    data.terrain.dirtColor = glm::vec3(0.5f, 0.5f, 0.55f);
    data.terrain.rockColor = glm::vec3(0.5f, 0.55f, 0.6f);
    data.terrain.cliffColor = glm::vec3(0.55f, 0.6f, 0.65f);
    data.terrain.grassColor = glm::vec3(0.4f, 0.5f, 0.45f);
    data.terrain.forestColor = glm::vec3(0.2f, 0.35f, 0.3f);
    data.terrain.jungleColor = glm::vec3(0.25f, 0.4f, 0.35f);
    data.terrain.shrubColor = glm::vec3(0.45f, 0.55f, 0.5f);
    data.terrain.snowColor = glm::vec3(0.97f, 0.98f, 1.0f);
    data.terrain.iceColor = glm::vec3(0.75f, 0.88f, 1.0f);
    data.terrain.glacierColor = glm::vec3(0.6f, 0.8f, 0.95f);
    data.terrain.lavaColor = glm::vec3(0.4f, 0.6f, 1.0f);
    data.terrain.ashColor = glm::vec3(0.5f, 0.55f, 0.6f);
    data.terrain.crystalColor = glm::vec3(0.7f, 0.85f, 1.0f);
    data.terrain.mossColor = glm::vec3(0.35f, 0.45f, 0.4f);

    // Vegetation - muted, cold colors
    data.vegetation.treeLeafColor = glm::vec3(0.2f, 0.35f, 0.3f);
    data.vegetation.treeBarkColor = glm::vec3(0.35f, 0.3f, 0.28f);
    data.vegetation.treeLeafVariation = glm::vec3(0.08f, 0.1f, 0.08f);
    data.vegetation.grassBaseColor = glm::vec3(0.35f, 0.45f, 0.4f);
    data.vegetation.grassTipColor = glm::vec3(0.5f, 0.6f, 0.55f);
    data.vegetation.grassVariation = glm::vec3(0.05f);
    data.vegetation.flowerColors[0] = glm::vec3(0.6f, 0.7f, 0.9f);
    data.vegetation.flowerColors[1] = glm::vec3(0.9f, 0.85f, 0.95f);
    data.vegetation.flowerColors[2] = glm::vec3(0.5f, 0.8f, 0.85f);
    data.vegetation.flowerColors[3] = glm::vec3(0.95f, 0.95f, 1.0f);
    data.vegetation.flowerColors[4] = glm::vec3(0.7f, 0.6f, 0.8f);
    data.vegetation.flowerColors[5] = glm::vec3(0.85f, 0.9f, 0.95f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.6f, 0.65f, 0.7f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.5f, 0.55f, 0.65f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.55f, 0.6f, 0.65f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.7f, 0.75f, 0.8f);
    data.vegetation.colorSaturation = 0.7f;
    data.vegetation.colorBrightness = 1.1f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.7f, 0.75f, 0.8f);
    data.creatures.carnivoreTint = glm::vec3(0.6f, 0.65f, 0.7f);
    data.creatures.aquaticTint = glm::vec3(0.5f, 0.6f, 0.7f);
    data.creatures.flyingTint = glm::vec3(0.75f, 0.8f, 0.85f);
    data.creatures.environmentInfluence = 0.4f;
    data.creatures.geneticVariation = 0.15f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.95f, 0.97f, 1.0f);
    data.colorGrading.saturation = 0.85f;
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createDesertWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Desert World";
    data.preset = PlanetPreset::DESERT_WORLD;
    data.seed = seed;

    // Atmosphere - hazy, warm
    data.atmosphere.skyZenithColor = glm::vec3(0.6f, 0.5f, 0.35f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.9f, 0.75f, 0.55f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.95f, 0.8f);
    data.atmosphere.moonColor = glm::vec3(0.9f, 0.85f, 0.75f);
    data.atmosphere.fogColor = glm::vec3(0.85f, 0.7f, 0.5f);
    data.atmosphere.ambientColor = glm::vec3(0.55f, 0.45f, 0.35f);
    data.atmosphere.fogDensity = 0.035f;
    data.atmosphere.fogStart = 50.0f;
    data.atmosphere.rayleighScattering = 0.002f;
    data.atmosphere.mieScattering = 0.003f;
    data.atmosphere.sunIntensity = 1.3f;
    data.atmosphere.moonIntensity = 0.2f;
    data.atmosphere.starVisibility = 0.85f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.9f, 0.6f, 0.3f);
    data.atmosphere.auroraColor2 = glm::vec3(0.8f, 0.4f, 0.2f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - arid, sandy
    data.terrain.deepWaterColor = glm::vec3(0.15f, 0.25f, 0.3f);
    data.terrain.shallowWaterColor = glm::vec3(0.3f, 0.5f, 0.45f);
    data.terrain.waterFoamColor = glm::vec3(0.95f, 0.9f, 0.85f);
    data.terrain.waterReflectionTint = glm::vec3(1.0f, 0.95f, 0.9f);
    data.terrain.sandColor = glm::vec3(0.85f, 0.7f, 0.5f);
    data.terrain.dirtColor = glm::vec3(0.6f, 0.45f, 0.3f);
    data.terrain.rockColor = glm::vec3(0.65f, 0.5f, 0.4f);
    data.terrain.cliffColor = glm::vec3(0.7f, 0.55f, 0.45f);
    data.terrain.grassColor = glm::vec3(0.5f, 0.45f, 0.25f);
    data.terrain.forestColor = glm::vec3(0.35f, 0.35f, 0.2f);
    data.terrain.jungleColor = glm::vec3(0.3f, 0.4f, 0.2f);
    data.terrain.shrubColor = glm::vec3(0.55f, 0.5f, 0.3f);
    data.terrain.snowColor = glm::vec3(0.95f, 0.93f, 0.88f);
    data.terrain.iceColor = glm::vec3(0.9f, 0.88f, 0.82f);
    data.terrain.glacierColor = glm::vec3(0.85f, 0.8f, 0.75f);
    data.terrain.lavaColor = glm::vec3(1.0f, 0.5f, 0.1f);
    data.terrain.ashColor = glm::vec3(0.5f, 0.45f, 0.4f);
    data.terrain.crystalColor = glm::vec3(0.9f, 0.75f, 0.5f);
    data.terrain.mossColor = glm::vec3(0.45f, 0.45f, 0.25f);

    // Vegetation - sparse, dry colors
    data.vegetation.treeLeafColor = glm::vec3(0.4f, 0.4f, 0.2f);
    data.vegetation.treeBarkColor = glm::vec3(0.45f, 0.35f, 0.25f);
    data.vegetation.treeLeafVariation = glm::vec3(0.1f, 0.08f, 0.05f);
    data.vegetation.grassBaseColor = glm::vec3(0.55f, 0.5f, 0.3f);
    data.vegetation.grassTipColor = glm::vec3(0.7f, 0.6f, 0.4f);
    data.vegetation.grassVariation = glm::vec3(0.08f, 0.06f, 0.04f);
    data.vegetation.flowerColors[0] = glm::vec3(0.9f, 0.3f, 0.3f);
    data.vegetation.flowerColors[1] = glm::vec3(1.0f, 0.6f, 0.2f);
    data.vegetation.flowerColors[2] = glm::vec3(0.9f, 0.8f, 0.3f);
    data.vegetation.flowerColors[3] = glm::vec3(0.7f, 0.4f, 0.6f);
    data.vegetation.flowerColors[4] = glm::vec3(0.95f, 0.9f, 0.8f);
    data.vegetation.flowerColors[5] = glm::vec3(0.6f, 0.8f, 0.4f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.7f, 0.6f, 0.5f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.6f, 0.5f, 0.4f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.55f, 0.5f, 0.45f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.75f, 0.65f, 0.55f);
    data.vegetation.colorSaturation = 0.85f;
    data.vegetation.colorBrightness = 1.1f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.7f, 0.6f, 0.45f);
    data.creatures.carnivoreTint = glm::vec3(0.6f, 0.5f, 0.4f);
    data.creatures.aquaticTint = glm::vec3(0.5f, 0.55f, 0.5f);
    data.creatures.flyingTint = glm::vec3(0.65f, 0.55f, 0.45f);
    data.creatures.environmentInfluence = 0.45f;
    data.creatures.geneticVariation = 0.18f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(1.0f, 0.95f, 0.88f);
    data.colorGrading.contrast = 1.1f;
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createOceanWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Ocean World";
    data.preset = PlanetPreset::OCEAN_WORLD;
    data.seed = seed;

    // Atmosphere - tropical, humid
    data.atmosphere.skyZenithColor = glm::vec3(0.25f, 0.5f, 0.8f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.5f, 0.75f, 0.9f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.98f, 0.92f);
    data.atmosphere.moonColor = glm::vec3(0.8f, 0.85f, 0.95f);
    data.atmosphere.fogColor = glm::vec3(0.6f, 0.75f, 0.85f);
    data.atmosphere.ambientColor = glm::vec3(0.45f, 0.55f, 0.6f);
    data.atmosphere.fogDensity = 0.02f;
    data.atmosphere.fogStart = 80.0f;
    data.atmosphere.rayleighScattering = 0.0028f;
    data.atmosphere.mieScattering = 0.0012f;
    data.atmosphere.sunIntensity = 1.05f;
    data.atmosphere.moonIntensity = 0.2f;
    data.atmosphere.starVisibility = 0.6f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.3f, 0.8f, 0.6f);
    data.atmosphere.auroraColor2 = glm::vec3(0.2f, 0.5f, 0.9f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - water dominant
    data.terrain.deepWaterColor = glm::vec3(0.0f, 0.12f, 0.25f);
    data.terrain.shallowWaterColor = glm::vec3(0.1f, 0.45f, 0.55f);
    data.terrain.waterFoamColor = glm::vec3(0.95f, 0.98f, 1.0f);
    data.terrain.waterReflectionTint = glm::vec3(0.95f, 1.0f, 1.0f);
    data.terrain.sandColor = glm::vec3(0.9f, 0.85f, 0.7f);
    data.terrain.dirtColor = glm::vec3(0.45f, 0.35f, 0.25f);
    data.terrain.rockColor = glm::vec3(0.45f, 0.5f, 0.5f);
    data.terrain.cliffColor = glm::vec3(0.5f, 0.55f, 0.55f);
    data.terrain.grassColor = glm::vec3(0.2f, 0.55f, 0.25f);
    data.terrain.forestColor = glm::vec3(0.1f, 0.4f, 0.15f);
    data.terrain.jungleColor = glm::vec3(0.05f, 0.45f, 0.15f);
    data.terrain.shrubColor = glm::vec3(0.25f, 0.5f, 0.3f);
    data.terrain.snowColor = glm::vec3(0.95f, 0.97f, 1.0f);
    data.terrain.iceColor = glm::vec3(0.85f, 0.92f, 1.0f);
    data.terrain.glacierColor = glm::vec3(0.75f, 0.88f, 0.98f);
    data.terrain.lavaColor = glm::vec3(1.0f, 0.35f, 0.05f);
    data.terrain.ashColor = glm::vec3(0.35f, 0.35f, 0.38f);
    data.terrain.crystalColor = glm::vec3(0.6f, 0.85f, 0.95f);
    data.terrain.mossColor = glm::vec3(0.15f, 0.45f, 0.2f);

    // Vegetation - lush, tropical
    data.vegetation.treeLeafColor = glm::vec3(0.1f, 0.5f, 0.15f);
    data.vegetation.treeBarkColor = glm::vec3(0.35f, 0.25f, 0.15f);
    data.vegetation.treeLeafVariation = glm::vec3(0.1f, 0.12f, 0.06f);
    data.vegetation.grassBaseColor = glm::vec3(0.15f, 0.5f, 0.2f);
    data.vegetation.grassTipColor = glm::vec3(0.3f, 0.65f, 0.35f);
    data.vegetation.grassVariation = glm::vec3(0.06f);
    data.vegetation.flowerColors[0] = glm::vec3(1.0f, 0.3f, 0.4f);
    data.vegetation.flowerColors[1] = glm::vec3(1.0f, 0.7f, 0.2f);
    data.vegetation.flowerColors[2] = glm::vec3(0.95f, 0.4f, 0.7f);
    data.vegetation.flowerColors[3] = glm::vec3(0.4f, 0.6f, 0.95f);
    data.vegetation.flowerColors[4] = glm::vec3(1.0f, 0.95f, 0.4f);
    data.vegetation.flowerColors[5] = glm::vec3(0.98f, 0.98f, 0.95f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.7f, 0.6f, 0.55f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.85f, 0.4f, 0.3f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.55f, 0.5f, 0.45f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.8f, 0.75f, 0.65f);
    data.vegetation.colorSaturation = 1.1f;
    data.vegetation.colorBrightness = 1.0f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.5f, 0.55f, 0.5f);
    data.creatures.carnivoreTint = glm::vec3(0.45f, 0.5f, 0.5f);
    data.creatures.aquaticTint = glm::vec3(0.35f, 0.55f, 0.65f);
    data.creatures.flyingTint = glm::vec3(0.55f, 0.6f, 0.6f);
    data.creatures.environmentInfluence = 0.35f;
    data.creatures.geneticVariation = 0.25f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.98f, 1.0f, 1.0f);
    data.colorGrading.saturation = 1.05f;
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createVolcanicWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Volcanic World";
    data.preset = PlanetPreset::VOLCANIC_WORLD;
    data.seed = seed;

    // Atmosphere - smoky, red-tinted
    data.atmosphere.skyZenithColor = glm::vec3(0.25f, 0.15f, 0.12f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.6f, 0.35f, 0.2f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.7f, 0.4f);
    data.atmosphere.moonColor = glm::vec3(0.8f, 0.6f, 0.5f);
    data.atmosphere.fogColor = glm::vec3(0.4f, 0.3f, 0.25f);
    data.atmosphere.ambientColor = glm::vec3(0.4f, 0.25f, 0.2f);
    data.atmosphere.fogDensity = 0.045f;
    data.atmosphere.fogStart = 40.0f;
    data.atmosphere.rayleighScattering = 0.001f;
    data.atmosphere.mieScattering = 0.004f;
    data.atmosphere.sunIntensity = 0.8f;
    data.atmosphere.moonIntensity = 0.1f;
    data.atmosphere.starVisibility = 0.4f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.9f, 0.4f, 0.2f);
    data.atmosphere.auroraColor2 = glm::vec3(0.8f, 0.2f, 0.1f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - lava, ash, dark rock
    data.terrain.deepWaterColor = glm::vec3(0.15f, 0.1f, 0.08f);
    data.terrain.shallowWaterColor = glm::vec3(0.3f, 0.2f, 0.15f);
    data.terrain.waterFoamColor = glm::vec3(0.7f, 0.6f, 0.5f);
    data.terrain.waterReflectionTint = glm::vec3(1.0f, 0.85f, 0.7f);
    data.terrain.sandColor = glm::vec3(0.35f, 0.3f, 0.28f);
    data.terrain.dirtColor = glm::vec3(0.25f, 0.2f, 0.18f);
    data.terrain.rockColor = glm::vec3(0.2f, 0.18f, 0.17f);
    data.terrain.cliffColor = glm::vec3(0.25f, 0.22f, 0.2f);
    data.terrain.grassColor = glm::vec3(0.25f, 0.22f, 0.15f);
    data.terrain.forestColor = glm::vec3(0.18f, 0.15f, 0.1f);
    data.terrain.jungleColor = glm::vec3(0.2f, 0.18f, 0.12f);
    data.terrain.shrubColor = glm::vec3(0.3f, 0.25f, 0.18f);
    data.terrain.snowColor = glm::vec3(0.6f, 0.58f, 0.55f);
    data.terrain.iceColor = glm::vec3(0.5f, 0.48f, 0.45f);
    data.terrain.glacierColor = glm::vec3(0.45f, 0.42f, 0.4f);
    data.terrain.lavaColor = glm::vec3(1.0f, 0.4f, 0.0f);
    data.terrain.ashColor = glm::vec3(0.28f, 0.26f, 0.25f);
    data.terrain.crystalColor = glm::vec3(0.9f, 0.4f, 0.2f);
    data.terrain.mossColor = glm::vec3(0.22f, 0.2f, 0.12f);

    // Vegetation - dark, hardy plants
    data.vegetation.treeLeafColor = glm::vec3(0.2f, 0.18f, 0.12f);
    data.vegetation.treeBarkColor = glm::vec3(0.15f, 0.12f, 0.1f);
    data.vegetation.treeLeafVariation = glm::vec3(0.05f);
    data.vegetation.grassBaseColor = glm::vec3(0.22f, 0.2f, 0.12f);
    data.vegetation.grassTipColor = glm::vec3(0.35f, 0.28f, 0.18f);
    data.vegetation.grassVariation = glm::vec3(0.04f);
    data.vegetation.flowerColors[0] = glm::vec3(1.0f, 0.5f, 0.1f);
    data.vegetation.flowerColors[1] = glm::vec3(0.9f, 0.2f, 0.1f);
    data.vegetation.flowerColors[2] = glm::vec3(0.8f, 0.6f, 0.2f);
    data.vegetation.flowerColors[3] = glm::vec3(0.6f, 0.4f, 0.3f);
    data.vegetation.flowerColors[4] = glm::vec3(0.5f, 0.35f, 0.3f);
    data.vegetation.flowerColors[5] = glm::vec3(0.7f, 0.5f, 0.35f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.4f, 0.35f, 0.3f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.35f, 0.28f, 0.25f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.3f, 0.25f, 0.22f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.45f, 0.38f, 0.32f);
    data.vegetation.colorSaturation = 0.75f;
    data.vegetation.colorBrightness = 0.85f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.4f, 0.35f, 0.3f);
    data.creatures.carnivoreTint = glm::vec3(0.35f, 0.28f, 0.25f);
    data.creatures.aquaticTint = glm::vec3(0.35f, 0.3f, 0.28f);
    data.creatures.flyingTint = glm::vec3(0.45f, 0.38f, 0.32f);
    data.creatures.environmentInfluence = 0.5f;
    data.creatures.geneticVariation = 0.15f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(1.0f, 0.9f, 0.8f);
    data.colorGrading.contrast = 1.15f;
    data.colorGrading.saturation = 0.9f;
    setupTimeVariants(data);

    // Override night for volcanic glow
    data.atmosphereNight.ambientColor = glm::vec3(0.3f, 0.15f, 0.1f);

    return data;
}

PlanetThemeData PlanetTheme::createBioluminescent(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Bioluminescent";
    data.preset = PlanetPreset::BIOLUMINESCENT;
    data.seed = seed;

    // Atmosphere - dark with glow
    data.atmosphere.skyZenithColor = glm::vec3(0.05f, 0.08f, 0.15f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.1f, 0.15f, 0.25f);
    data.atmosphere.sunColor = glm::vec3(0.7f, 0.8f, 1.0f);
    data.atmosphere.moonColor = glm::vec3(0.5f, 0.7f, 0.9f);
    data.atmosphere.fogColor = glm::vec3(0.08f, 0.12f, 0.2f);
    data.atmosphere.ambientColor = glm::vec3(0.15f, 0.2f, 0.3f);
    data.atmosphere.fogDensity = 0.025f;
    data.atmosphere.fogStart = 60.0f;
    data.atmosphere.rayleighScattering = 0.001f;
    data.atmosphere.mieScattering = 0.002f;
    data.atmosphere.sunIntensity = 0.5f;
    data.atmosphere.moonIntensity = 0.4f;
    data.atmosphere.starVisibility = 0.95f;
    data.atmosphere.hasAurora = true;
    data.atmosphere.auroraColor1 = glm::vec3(0.2f, 0.9f, 0.6f);
    data.atmosphere.auroraColor2 = glm::vec3(0.4f, 0.5f, 0.95f);
    data.atmosphere.auroraIntensity = 0.7f;

    // Terrain - dark with glowing elements
    data.terrain.deepWaterColor = glm::vec3(0.02f, 0.05f, 0.12f);
    data.terrain.shallowWaterColor = glm::vec3(0.05f, 0.15f, 0.25f);
    data.terrain.waterFoamColor = glm::vec3(0.3f, 0.6f, 0.8f);
    data.terrain.waterReflectionTint = glm::vec3(0.5f, 0.8f, 1.0f);
    data.terrain.sandColor = glm::vec3(0.2f, 0.22f, 0.25f);
    data.terrain.dirtColor = glm::vec3(0.12f, 0.13f, 0.15f);
    data.terrain.rockColor = glm::vec3(0.15f, 0.16f, 0.18f);
    data.terrain.cliffColor = glm::vec3(0.18f, 0.19f, 0.22f);
    data.terrain.grassColor = glm::vec3(0.1f, 0.3f, 0.25f);
    data.terrain.forestColor = glm::vec3(0.08f, 0.22f, 0.18f);
    data.terrain.jungleColor = glm::vec3(0.06f, 0.25f, 0.2f);
    data.terrain.shrubColor = glm::vec3(0.12f, 0.35f, 0.28f);
    data.terrain.snowColor = glm::vec3(0.6f, 0.7f, 0.8f);
    data.terrain.iceColor = glm::vec3(0.4f, 0.55f, 0.7f);
    data.terrain.glacierColor = glm::vec3(0.35f, 0.5f, 0.65f);
    data.terrain.lavaColor = glm::vec3(0.3f, 0.8f, 0.6f);
    data.terrain.ashColor = glm::vec3(0.15f, 0.16f, 0.18f);
    data.terrain.crystalColor = glm::vec3(0.4f, 0.9f, 0.8f);
    data.terrain.mossColor = glm::vec3(0.1f, 0.35f, 0.25f);

    // Vegetation - glowing
    data.vegetation.treeLeafColor = glm::vec3(0.15f, 0.45f, 0.35f);
    data.vegetation.treeBarkColor = glm::vec3(0.1f, 0.12f, 0.15f);
    data.vegetation.treeLeafVariation = glm::vec3(0.1f, 0.15f, 0.12f);
    data.vegetation.grassBaseColor = glm::vec3(0.1f, 0.35f, 0.3f);
    data.vegetation.grassTipColor = glm::vec3(0.2f, 0.6f, 0.5f);
    data.vegetation.grassVariation = glm::vec3(0.08f, 0.12f, 0.1f);
    data.vegetation.flowerColors[0] = glm::vec3(0.3f, 1.0f, 0.8f);
    data.vegetation.flowerColors[1] = glm::vec3(0.5f, 0.8f, 1.0f);
    data.vegetation.flowerColors[2] = glm::vec3(0.8f, 0.4f, 1.0f);
    data.vegetation.flowerColors[3] = glm::vec3(1.0f, 0.6f, 0.8f);
    data.vegetation.flowerColors[4] = glm::vec3(0.6f, 1.0f, 0.5f);
    data.vegetation.flowerColors[5] = glm::vec3(0.4f, 0.6f, 1.0f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.3f, 0.7f, 0.6f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.5f, 0.4f, 0.8f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.2f, 0.5f, 0.7f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.4f, 0.8f, 0.5f);
    data.vegetation.colorSaturation = 1.3f;
    data.vegetation.colorBrightness = 1.2f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.3f, 0.5f, 0.45f);
    data.creatures.carnivoreTint = glm::vec3(0.25f, 0.4f, 0.5f);
    data.creatures.aquaticTint = glm::vec3(0.2f, 0.45f, 0.55f);
    data.creatures.flyingTint = glm::vec3(0.35f, 0.55f, 0.5f);
    data.creatures.environmentInfluence = 0.4f;
    data.creatures.geneticVariation = 0.25f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.9f, 1.0f, 1.0f);
    data.colorGrading.saturation = 1.2f;
    data.colorGrading.brightness = 0.9f;
    setupTimeVariants(data);

    // Night is actually brighter due to bioluminescence
    data.atmosphereNight.ambientColor = glm::vec3(0.2f, 0.3f, 0.35f);

    return data;
}

PlanetThemeData PlanetTheme::createCrystalWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Crystal World";
    data.preset = PlanetPreset::CRYSTAL_WORLD;
    data.seed = seed;

    // Atmosphere - prismatic, bright
    data.atmosphere.skyZenithColor = glm::vec3(0.5f, 0.4f, 0.7f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.8f, 0.75f, 0.9f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 1.0f, 1.0f);
    data.atmosphere.moonColor = glm::vec3(0.85f, 0.9f, 1.0f);
    data.atmosphere.fogColor = glm::vec3(0.75f, 0.7f, 0.85f);
    data.atmosphere.ambientColor = glm::vec3(0.55f, 0.5f, 0.6f);
    data.atmosphere.fogDensity = 0.015f;
    data.atmosphere.fogStart = 100.0f;
    data.atmosphere.rayleighScattering = 0.003f;
    data.atmosphere.mieScattering = 0.001f;
    data.atmosphere.sunIntensity = 1.1f;
    data.atmosphere.moonIntensity = 0.3f;
    data.atmosphere.starVisibility = 0.75f;
    data.atmosphere.hasAurora = true;
    data.atmosphere.auroraColor1 = glm::vec3(0.7f, 0.4f, 0.9f);
    data.atmosphere.auroraColor2 = glm::vec3(0.4f, 0.8f, 0.9f);
    data.atmosphere.auroraIntensity = 0.45f;

    // Terrain - crystalline, reflective
    data.terrain.deepWaterColor = glm::vec3(0.15f, 0.2f, 0.35f);
    data.terrain.shallowWaterColor = glm::vec3(0.35f, 0.5f, 0.65f);
    data.terrain.waterFoamColor = glm::vec3(0.95f, 0.95f, 1.0f);
    data.terrain.waterReflectionTint = glm::vec3(1.0f, 0.98f, 1.0f);
    data.terrain.sandColor = glm::vec3(0.8f, 0.78f, 0.85f);
    data.terrain.dirtColor = glm::vec3(0.5f, 0.48f, 0.55f);
    data.terrain.rockColor = glm::vec3(0.55f, 0.52f, 0.6f);
    data.terrain.cliffColor = glm::vec3(0.6f, 0.58f, 0.65f);
    data.terrain.grassColor = glm::vec3(0.4f, 0.55f, 0.5f);
    data.terrain.forestColor = glm::vec3(0.3f, 0.45f, 0.42f);
    data.terrain.jungleColor = glm::vec3(0.28f, 0.48f, 0.4f);
    data.terrain.shrubColor = glm::vec3(0.45f, 0.58f, 0.52f);
    data.terrain.snowColor = glm::vec3(0.98f, 0.98f, 1.0f);
    data.terrain.iceColor = glm::vec3(0.85f, 0.9f, 1.0f);
    data.terrain.glacierColor = glm::vec3(0.75f, 0.85f, 0.98f);
    data.terrain.lavaColor = glm::vec3(0.9f, 0.5f, 0.9f);
    data.terrain.ashColor = glm::vec3(0.45f, 0.42f, 0.48f);
    data.terrain.crystalColor = glm::vec3(0.8f, 0.7f, 0.95f);
    data.terrain.mossColor = glm::vec3(0.35f, 0.5f, 0.45f);

    // Vegetation - crystalline tints
    data.vegetation.treeLeafColor = glm::vec3(0.35f, 0.5f, 0.48f);
    data.vegetation.treeBarkColor = glm::vec3(0.4f, 0.38f, 0.42f);
    data.vegetation.treeLeafVariation = glm::vec3(0.1f, 0.12f, 0.1f);
    data.vegetation.grassBaseColor = glm::vec3(0.38f, 0.52f, 0.48f);
    data.vegetation.grassTipColor = glm::vec3(0.55f, 0.7f, 0.65f);
    data.vegetation.grassVariation = glm::vec3(0.06f);
    data.vegetation.flowerColors[0] = glm::vec3(0.9f, 0.5f, 0.9f);
    data.vegetation.flowerColors[1] = glm::vec3(0.5f, 0.8f, 0.95f);
    data.vegetation.flowerColors[2] = glm::vec3(0.95f, 0.8f, 0.5f);
    data.vegetation.flowerColors[3] = glm::vec3(0.6f, 0.95f, 0.7f);
    data.vegetation.flowerColors[4] = glm::vec3(0.95f, 0.6f, 0.6f);
    data.vegetation.flowerColors[5] = glm::vec3(0.98f, 0.98f, 0.95f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.7f, 0.65f, 0.75f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.6f, 0.7f, 0.8f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.55f, 0.6f, 0.65f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.75f, 0.7f, 0.8f);
    data.vegetation.colorSaturation = 0.95f;
    data.vegetation.colorBrightness = 1.1f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.6f, 0.58f, 0.65f);
    data.creatures.carnivoreTint = glm::vec3(0.52f, 0.5f, 0.58f);
    data.creatures.aquaticTint = glm::vec3(0.5f, 0.58f, 0.68f);
    data.creatures.flyingTint = glm::vec3(0.65f, 0.62f, 0.7f);
    data.creatures.environmentInfluence = 0.3f;
    data.creatures.geneticVariation = 0.2f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.98f, 0.97f, 1.0f);
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createToxicWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Toxic World";
    data.preset = PlanetPreset::TOXIC_WORLD;
    data.seed = seed;

    // Atmosphere - sickly green/yellow
    data.atmosphere.skyZenithColor = glm::vec3(0.35f, 0.4f, 0.15f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.6f, 0.65f, 0.3f);
    data.atmosphere.sunColor = glm::vec3(0.95f, 1.0f, 0.7f);
    data.atmosphere.moonColor = glm::vec3(0.7f, 0.8f, 0.5f);
    data.atmosphere.fogColor = glm::vec3(0.5f, 0.55f, 0.25f);
    data.atmosphere.ambientColor = glm::vec3(0.4f, 0.45f, 0.25f);
    data.atmosphere.fogDensity = 0.04f;
    data.atmosphere.fogStart = 45.0f;
    data.atmosphere.rayleighScattering = 0.002f;
    data.atmosphere.mieScattering = 0.003f;
    data.atmosphere.sunIntensity = 0.75f;
    data.atmosphere.moonIntensity = 0.15f;
    data.atmosphere.starVisibility = 0.35f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.6f, 0.9f, 0.3f);
    data.atmosphere.auroraColor2 = glm::vec3(0.4f, 0.7f, 0.2f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - toxic, corroded
    data.terrain.deepWaterColor = glm::vec3(0.2f, 0.25f, 0.1f);
    data.terrain.shallowWaterColor = glm::vec3(0.4f, 0.5f, 0.2f);
    data.terrain.waterFoamColor = glm::vec3(0.7f, 0.75f, 0.5f);
    data.terrain.waterReflectionTint = glm::vec3(0.9f, 1.0f, 0.7f);
    data.terrain.sandColor = glm::vec3(0.55f, 0.5f, 0.35f);
    data.terrain.dirtColor = glm::vec3(0.35f, 0.32f, 0.22f);
    data.terrain.rockColor = glm::vec3(0.38f, 0.36f, 0.28f);
    data.terrain.cliffColor = glm::vec3(0.42f, 0.4f, 0.32f);
    data.terrain.grassColor = glm::vec3(0.5f, 0.55f, 0.25f);
    data.terrain.forestColor = glm::vec3(0.35f, 0.42f, 0.18f);
    data.terrain.jungleColor = glm::vec3(0.38f, 0.48f, 0.2f);
    data.terrain.shrubColor = glm::vec3(0.55f, 0.6f, 0.28f);
    data.terrain.snowColor = glm::vec3(0.8f, 0.82f, 0.7f);
    data.terrain.iceColor = glm::vec3(0.65f, 0.7f, 0.55f);
    data.terrain.glacierColor = glm::vec3(0.55f, 0.62f, 0.48f);
    data.terrain.lavaColor = glm::vec3(0.8f, 0.9f, 0.2f);
    data.terrain.ashColor = glm::vec3(0.38f, 0.36f, 0.3f);
    data.terrain.crystalColor = glm::vec3(0.6f, 0.8f, 0.3f);
    data.terrain.mossColor = glm::vec3(0.45f, 0.5f, 0.22f);

    // Vegetation - sickly, toxic
    data.vegetation.treeLeafColor = glm::vec3(0.45f, 0.52f, 0.2f);
    data.vegetation.treeBarkColor = glm::vec3(0.3f, 0.28f, 0.2f);
    data.vegetation.treeLeafVariation = glm::vec3(0.1f, 0.08f, 0.05f);
    data.vegetation.grassBaseColor = glm::vec3(0.48f, 0.55f, 0.22f);
    data.vegetation.grassTipColor = glm::vec3(0.65f, 0.72f, 0.35f);
    data.vegetation.grassVariation = glm::vec3(0.08f, 0.06f, 0.04f);
    data.vegetation.flowerColors[0] = glm::vec3(0.8f, 0.9f, 0.3f);
    data.vegetation.flowerColors[1] = glm::vec3(0.6f, 0.8f, 0.2f);
    data.vegetation.flowerColors[2] = glm::vec3(0.9f, 0.7f, 0.3f);
    data.vegetation.flowerColors[3] = glm::vec3(0.7f, 0.5f, 0.7f);
    data.vegetation.flowerColors[4] = glm::vec3(0.5f, 0.7f, 0.4f);
    data.vegetation.flowerColors[5] = glm::vec3(0.75f, 0.8f, 0.5f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.55f, 0.5f, 0.35f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.6f, 0.65f, 0.3f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.45f, 0.42f, 0.3f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.65f, 0.6f, 0.4f);
    data.vegetation.colorSaturation = 0.9f;
    data.vegetation.colorBrightness = 0.95f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.5f, 0.52f, 0.38f);
    data.creatures.carnivoreTint = glm::vec3(0.45f, 0.45f, 0.32f);
    data.creatures.aquaticTint = glm::vec3(0.42f, 0.5f, 0.35f);
    data.creatures.flyingTint = glm::vec3(0.55f, 0.58f, 0.4f);
    data.creatures.environmentInfluence = 0.5f;
    data.creatures.geneticVariation = 0.2f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.95f, 1.0f, 0.85f);
    data.colorGrading.saturation = 0.95f;
    setupTimeVariants(data);

    return data;
}

PlanetThemeData PlanetTheme::createAncientWorld(uint32_t seed) {
    PlanetThemeData data;
    data.name = "Ancient World";
    data.preset = PlanetPreset::ANCIENT_WORLD;
    data.seed = seed;

    // Atmosphere - muted, nostalgic
    data.atmosphere.skyZenithColor = glm::vec3(0.35f, 0.42f, 0.5f);
    data.atmosphere.skyHorizonColor = glm::vec3(0.6f, 0.65f, 0.65f);
    data.atmosphere.sunColor = glm::vec3(1.0f, 0.95f, 0.85f);
    data.atmosphere.moonColor = glm::vec3(0.8f, 0.82f, 0.85f);
    data.atmosphere.fogColor = glm::vec3(0.55f, 0.58f, 0.55f);
    data.atmosphere.ambientColor = glm::vec3(0.45f, 0.47f, 0.45f);
    data.atmosphere.fogDensity = 0.022f;
    data.atmosphere.fogStart = 70.0f;
    data.atmosphere.rayleighScattering = 0.0022f;
    data.atmosphere.mieScattering = 0.0015f;
    data.atmosphere.sunIntensity = 0.85f;
    data.atmosphere.moonIntensity = 0.2f;
    data.atmosphere.starVisibility = 0.7f;
    data.atmosphere.hasAurora = false;
    data.atmosphere.auroraColor1 = glm::vec3(0.4f, 0.6f, 0.5f);
    data.atmosphere.auroraColor2 = glm::vec3(0.3f, 0.5f, 0.6f);
    data.atmosphere.auroraIntensity = 0.0f;

    // Terrain - weathered, mossy
    data.terrain.deepWaterColor = glm::vec3(0.1f, 0.15f, 0.18f);
    data.terrain.shallowWaterColor = glm::vec3(0.25f, 0.35f, 0.38f);
    data.terrain.waterFoamColor = glm::vec3(0.85f, 0.88f, 0.85f);
    data.terrain.waterReflectionTint = glm::vec3(0.95f, 0.97f, 0.95f);
    data.terrain.sandColor = glm::vec3(0.65f, 0.6f, 0.52f);
    data.terrain.dirtColor = glm::vec3(0.4f, 0.35f, 0.28f);
    data.terrain.rockColor = glm::vec3(0.45f, 0.43f, 0.4f);
    data.terrain.cliffColor = glm::vec3(0.5f, 0.48f, 0.45f);
    data.terrain.grassColor = glm::vec3(0.3f, 0.42f, 0.25f);
    data.terrain.forestColor = glm::vec3(0.2f, 0.32f, 0.18f);
    data.terrain.jungleColor = glm::vec3(0.18f, 0.35f, 0.18f);
    data.terrain.shrubColor = glm::vec3(0.35f, 0.45f, 0.28f);
    data.terrain.snowColor = glm::vec3(0.9f, 0.92f, 0.9f);
    data.terrain.iceColor = glm::vec3(0.78f, 0.82f, 0.85f);
    data.terrain.glacierColor = glm::vec3(0.68f, 0.75f, 0.8f);
    data.terrain.lavaColor = glm::vec3(0.9f, 0.4f, 0.15f);
    data.terrain.ashColor = glm::vec3(0.4f, 0.38f, 0.36f);
    data.terrain.crystalColor = glm::vec3(0.6f, 0.65f, 0.7f);
    data.terrain.mossColor = glm::vec3(0.28f, 0.4f, 0.22f);

    // Vegetation - old growth, mossy
    data.vegetation.treeLeafColor = glm::vec3(0.22f, 0.38f, 0.2f);
    data.vegetation.treeBarkColor = glm::vec3(0.32f, 0.28f, 0.22f);
    data.vegetation.treeLeafVariation = glm::vec3(0.08f, 0.1f, 0.06f);
    data.vegetation.grassBaseColor = glm::vec3(0.28f, 0.4f, 0.22f);
    data.vegetation.grassTipColor = glm::vec3(0.42f, 0.55f, 0.35f);
    data.vegetation.grassVariation = glm::vec3(0.05f);
    data.vegetation.flowerColors[0] = glm::vec3(0.75f, 0.6f, 0.5f);
    data.vegetation.flowerColors[1] = glm::vec3(0.6f, 0.55f, 0.7f);
    data.vegetation.flowerColors[2] = glm::vec3(0.8f, 0.75f, 0.55f);
    data.vegetation.flowerColors[3] = glm::vec3(0.55f, 0.65f, 0.7f);
    data.vegetation.flowerColors[4] = glm::vec3(0.7f, 0.5f, 0.55f);
    data.vegetation.flowerColors[5] = glm::vec3(0.85f, 0.85f, 0.8f);
    data.vegetation.mushroomColors[0] = glm::vec3(0.6f, 0.55f, 0.5f);
    data.vegetation.mushroomColors[1] = glm::vec3(0.5f, 0.45f, 0.4f);
    data.vegetation.mushroomColors[2] = glm::vec3(0.55f, 0.52f, 0.48f);
    data.vegetation.mushroomColors[3] = glm::vec3(0.65f, 0.6f, 0.55f);
    data.vegetation.colorSaturation = 0.85f;
    data.vegetation.colorBrightness = 0.95f;

    // Creatures
    data.creatures.herbivoreBaseTint = glm::vec3(0.55f, 0.52f, 0.45f);
    data.creatures.carnivoreTint = glm::vec3(0.48f, 0.45f, 0.4f);
    data.creatures.aquaticTint = glm::vec3(0.42f, 0.48f, 0.5f);
    data.creatures.flyingTint = glm::vec3(0.52f, 0.5f, 0.48f);
    data.creatures.environmentInfluence = 0.35f;
    data.creatures.geneticVariation = 0.18f;

    setupDefaultColorGrading(data.colorGrading);
    data.colorGrading.colorFilter = glm::vec3(0.98f, 0.97f, 0.95f);
    data.colorGrading.saturation = 0.9f;
    data.colorGrading.contrast = 0.95f;
    setupTimeVariants(data);

    return data;
}
