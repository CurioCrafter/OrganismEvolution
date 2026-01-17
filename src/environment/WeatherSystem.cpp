#include "WeatherSystem.h"
#include "SeasonManager.h"
#include "ClimateSystem.h"
#include <cmath>
#include <algorithm>
#include <random>

// Random generator for weather
static std::mt19937 weatherRng(std::random_device{}());

WeatherSystem::WeatherSystem() {
    currentState = createStateForWeather(WeatherType::CLEAR);
    targetState = currentState;
}

void WeatherSystem::initialize(SeasonManager* season, ClimateSystem* climate) {
    seasonManager = season;
    climateSystem = climate;

    // Set initial weather based on season
    WeatherType initialWeather = getRandomWeatherForSeason();
    currentState = createStateForWeather(initialWeather);
    targetState = currentState;
}

void WeatherSystem::update(float deltaTime) {
    weatherTimer += deltaTime;

    // Update transition if active
    if (transition.isTransitioning) {
        updateTransition(deltaTime);
    }

    // Check for automatic weather change
    if (autoWeatherChange && weatherTimer >= weatherChangeInterval && !transition.isTransitioning) {
        WeatherType newWeather = determineWeatherForConditions();
        if (newWeather != currentState.type) {
            setWeather(newWeather);
        }
        weatherTimer = 0.0f;
    }

    // Update weather-specific effects
    updateLightning(deltaTime);
    updateGroundWetness(deltaTime);
    updateSkyColors();
}

void WeatherSystem::setWeather(WeatherType type, float transitionDuration) {
    if (type == currentState.type && !transition.isTransitioning) {
        return; // Already this weather
    }

    transition.fromWeather = currentState.type;
    transition.toWeather = type;
    transition.duration = transitionDuration;
    transition.progress = 0.0f;
    transition.isTransitioning = true;

    targetState = createStateForWeather(type);

    if (onWeatherChange) {
        onWeatherChange(type);
    }
}

bool WeatherSystem::isRaining() const {
    return currentState.type == WeatherType::RAIN_LIGHT ||
           currentState.type == WeatherType::RAIN_HEAVY ||
           currentState.type == WeatherType::THUNDERSTORM;
}

bool WeatherSystem::isSnowing() const {
    return currentState.type == WeatherType::SNOW_LIGHT ||
           currentState.type == WeatherType::SNOW_HEAVY;
}

bool WeatherSystem::isPrecipitating() const {
    return isRaining() || isSnowing();
}

bool WeatherSystem::isFoggy() const {
    return currentState.type == WeatherType::FOG ||
           currentState.type == WeatherType::MIST ||
           currentState.fogDensity > 0.3f;
}

bool WeatherSystem::isStormy() const {
    return currentState.type == WeatherType::THUNDERSTORM ||
           currentState.type == WeatherType::SNOW_HEAVY ||
           currentState.type == WeatherType::SANDSTORM;
}

float WeatherSystem::getVisibility() const {
    // Base visibility in clear weather: 1000 units
    float baseVisibility = 1000.0f;

    // Reduce based on fog
    float fogFactor = 1.0f - currentState.fogDensity * 0.9f;

    // Reduce based on precipitation
    float precipFactor = 1.0f - currentState.precipitationIntensity * 0.5f;

    return baseVisibility * fogFactor * precipFactor;
}

float WeatherSystem::getSunIntensity() const {
    return currentState.sunIntensity;
}

float WeatherSystem::getWindStrength() const {
    return currentState.windStrength;
}

bool WeatherSystem::hasLightningFlash() const {
    return hasRecentLightning && currentState.lightningIntensity > 0.5f;
}

glm::vec3 WeatherSystem::getLightningPosition() const {
    return lastLightningPos;
}

const char* WeatherSystem::getWeatherName(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR:         return "Clear";
        case WeatherType::PARTLY_CLOUDY: return "Partly Cloudy";
        case WeatherType::OVERCAST:      return "Overcast";
        case WeatherType::RAIN_LIGHT:    return "Light Rain";
        case WeatherType::RAIN_HEAVY:    return "Heavy Rain";
        case WeatherType::THUNDERSTORM:  return "Thunderstorm";
        case WeatherType::SNOW_LIGHT:    return "Light Snow";
        case WeatherType::SNOW_HEAVY:    return "Heavy Snow";
        case WeatherType::FOG:           return "Fog";
        case WeatherType::MIST:          return "Mist";
        case WeatherType::SANDSTORM:     return "Sandstorm";
        case WeatherType::WINDY:         return "Windy";
        default:                         return "Unknown";
    }
}

WeatherState WeatherSystem::getInterpolatedWeather() const {
    if (transition.isTransitioning) {
        return interpolateStates(
            createStateForWeather(transition.fromWeather),
            targetState,
            transition.progress
        );
    }
    return currentState;
}

// ============================================================================
// Private Methods
// ============================================================================

WeatherType WeatherSystem::determineWeatherForConditions() const {
    // Query climate system for local conditions
    float localTemperature = 15.0f;  // Default: temperate (Celsius)
    float localMoisture = 0.5f;      // Default: moderate
    ClimateBiome biome = ClimateBiome::TEMPERATE_FOREST;

    if (climateSystem) {
        // Get climate at world center (representative for global weather)
        ClimateData climate = climateSystem->getClimateAt(0.0f, 0.0f);

        // Convert normalized temperature (0-1) to Celsius (-30 to +40)
        localTemperature = climate.temperature * 70.0f - 30.0f;
        localMoisture = climate.moisture;
        biome = climate.getBiome();
    }

    // Get base seasonal weather probabilities
    Season season = Season::SUMMER;
    if (seasonManager) {
        season = seasonManager->getCurrentSeason();
    }

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float roll = dist(weatherRng);

    // ==========================================================================
    // Climate-aware weather filtering
    // ==========================================================================

    // Hot climate constraints (temperature > 25C): No snow, no blizzard
    bool isHotClimate = localTemperature > 25.0f;

    // Cold climate constraints (temperature < 5C): No thunderstorm, allow snow
    bool isColdClimate = localTemperature < 5.0f;

    // Freezing climate (temperature < -5C): Snow more likely
    bool isFreezingClimate = localTemperature < -5.0f;

    // Dry climate constraints (moisture < 0.3): More clear, less rain
    bool isDryClimate = localMoisture < 0.3f;

    // Wet climate benefits (moisture > 0.7): More rain, thunderstorm, fog
    bool isWetClimate = localMoisture > 0.7f;

    // Very dry (desert): Possible sandstorms
    bool isDesertClimate = localMoisture < 0.2f && localTemperature > 20.0f;

    // ==========================================================================
    // Biome-specific weather patterns
    // ==========================================================================

    // Desert biomes: Hot/dry weather, sandstorms
    if (biome == ClimateBiome::DESERT_HOT || biome == ClimateBiome::DESERT_COLD) {
        if (roll < 0.05f && biome == ClimateBiome::DESERT_HOT) {
            return WeatherType::SANDSTORM;
        }
        if (roll < 0.15f) return WeatherType::WINDY;
        if (roll < 0.20f) return WeatherType::PARTLY_CLOUDY;
        return WeatherType::CLEAR;  // Desert is mostly clear
    }

    // Tropical biomes: Frequent rain, thunderstorms
    if (biome == ClimateBiome::TROPICAL_RAINFOREST || biome == ClimateBiome::TROPICAL_SEASONAL) {
        if (roll < 0.25f) return WeatherType::THUNDERSTORM;
        if (roll < 0.45f) return WeatherType::RAIN_HEAVY;
        if (roll < 0.60f) return WeatherType::RAIN_LIGHT;
        if (roll < 0.70f) return WeatherType::PARTLY_CLOUDY;
        if (roll < 0.80f) return WeatherType::MIST;
        return WeatherType::CLEAR;
    }

    // Polar/Ice biomes: Snow, blizzards
    if (biome == ClimateBiome::ICE || biome == ClimateBiome::TUNDRA || biome == ClimateBiome::MOUNTAIN_SNOW) {
        if (roll < 0.20f) return WeatherType::SNOW_HEAVY;
        if (roll < 0.45f) return WeatherType::SNOW_LIGHT;
        if (roll < 0.55f) return WeatherType::OVERCAST;
        if (roll < 0.65f) return WeatherType::FOG;
        if (roll < 0.75f) return WeatherType::WINDY;
        return WeatherType::CLEAR;
    }

    // Swamp/wetlands: Fog, mist, rain
    if (biome == ClimateBiome::SWAMP) {
        if (roll < 0.20f) return WeatherType::FOG;
        if (roll < 0.35f) return WeatherType::MIST;
        if (roll < 0.50f) return WeatherType::RAIN_LIGHT;
        if (roll < 0.60f) return WeatherType::RAIN_HEAVY;
        if (roll < 0.70f) return WeatherType::OVERCAST;
        return WeatherType::PARTLY_CLOUDY;
    }

    // Boreal forest: Cold weather patterns
    if (biome == ClimateBiome::BOREAL_FOREST) {
        if (season == Season::WINTER) {
            if (roll < 0.30f) return WeatherType::SNOW_LIGHT;
            if (roll < 0.45f) return WeatherType::SNOW_HEAVY;
            if (roll < 0.60f) return WeatherType::OVERCAST;
            if (roll < 0.70f) return WeatherType::FOG;
            return WeatherType::CLEAR;
        }
        // Other seasons
        if (roll < 0.15f) return WeatherType::RAIN_LIGHT;
        if (roll < 0.25f) return WeatherType::OVERCAST;
        if (roll < 0.35f) return WeatherType::FOG;
        if (roll < 0.45f) return WeatherType::PARTLY_CLOUDY;
        return WeatherType::CLEAR;
    }

    // ==========================================================================
    // Generic climate-constrained seasonal weather (for temperate biomes)
    // ==========================================================================

    // Apply climate constraints to seasonal weather
    WeatherType candidate = getRandomWeatherForSeason();

    // Filter out impossible weather for climate
    int attempts = 0;
    const int maxAttempts = 10;

    while (attempts < maxAttempts) {
        bool weatherValid = true;

        // Hot climate: No snow
        if (isHotClimate && (candidate == WeatherType::SNOW_LIGHT ||
                            candidate == WeatherType::SNOW_HEAVY)) {
            weatherValid = false;
        }

        // Cold climate: No thunderstorms (not enough convection)
        if (isColdClimate && candidate == WeatherType::THUNDERSTORM) {
            weatherValid = false;
        }

        // Dry climate: Reduce rain probability significantly
        if (isDryClimate && (candidate == WeatherType::RAIN_LIGHT ||
                            candidate == WeatherType::RAIN_HEAVY ||
                            candidate == WeatherType::THUNDERSTORM)) {
            // 70% chance to reject rain in dry climates
            if (dist(weatherRng) < 0.7f) {
                weatherValid = false;
            }
        }

        // Desert climate: Allow sandstorms
        if (isDesertClimate && dist(weatherRng) < 0.1f) {
            return WeatherType::SANDSTORM;
        }

        // Wet climate: Boost rain/storm probability
        if (isWetClimate && candidate == WeatherType::CLEAR) {
            // 50% chance to reject clear weather in wet climates
            if (dist(weatherRng) < 0.5f) {
                weatherValid = false;
            }
        }

        // Freezing climate: Convert rain to snow
        if (isFreezingClimate) {
            if (candidate == WeatherType::RAIN_LIGHT) {
                return WeatherType::SNOW_LIGHT;
            }
            if (candidate == WeatherType::RAIN_HEAVY ||
                candidate == WeatherType::THUNDERSTORM) {
                return WeatherType::SNOW_HEAVY;
            }
        }

        if (weatherValid) {
            return candidate;
        }

        // Try again with new random weather
        candidate = getRandomWeatherForSeason();
        attempts++;
    }

    // Fallback: Return climate-appropriate default
    if (isHotClimate) return WeatherType::CLEAR;
    if (isColdClimate) return WeatherType::OVERCAST;
    if (isDryClimate) return WeatherType::CLEAR;
    if (isWetClimate) return WeatherType::RAIN_LIGHT;

    return candidate;
}

WeatherType WeatherSystem::getRandomWeatherForSeason() const {
    Season season = Season::SUMMER;
    float temperature = 0.5f;

    if (seasonManager) {
        season = seasonManager->getCurrentSeason();
        temperature = seasonManager->getTemperature();
    }

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float roll = dist(weatherRng);

    // Weather probabilities by season
    switch (season) {
        case Season::SPRING:
            if (roll < 0.25f) return WeatherType::RAIN_LIGHT;
            if (roll < 0.35f) return WeatherType::RAIN_HEAVY;
            if (roll < 0.45f) return WeatherType::PARTLY_CLOUDY;
            if (roll < 0.55f) return WeatherType::OVERCAST;
            if (roll < 0.60f) return WeatherType::MIST;
            if (roll < 0.65f) return WeatherType::THUNDERSTORM;
            return WeatherType::CLEAR;

        case Season::SUMMER:
            if (roll < 0.15f) return WeatherType::THUNDERSTORM;
            if (roll < 0.25f) return WeatherType::PARTLY_CLOUDY;
            if (roll < 0.30f) return WeatherType::RAIN_LIGHT;
            if (roll < 0.35f) return WeatherType::RAIN_HEAVY;
            return WeatherType::CLEAR;

        case Season::FALL:
            if (roll < 0.20f) return WeatherType::RAIN_LIGHT;
            if (roll < 0.35f) return WeatherType::RAIN_HEAVY;
            if (roll < 0.50f) return WeatherType::OVERCAST;
            if (roll < 0.60f) return WeatherType::FOG;
            if (roll < 0.65f) return WeatherType::WINDY;
            if (roll < 0.70f) return WeatherType::PARTLY_CLOUDY;
            return WeatherType::CLEAR;

        case Season::WINTER:
            if (temperature < 0.3f) {
                // Cold winter
                if (roll < 0.25f) return WeatherType::SNOW_HEAVY;
                if (roll < 0.45f) return WeatherType::SNOW_LIGHT;
                if (roll < 0.55f) return WeatherType::OVERCAST;
                if (roll < 0.65f) return WeatherType::FOG;
                return WeatherType::CLEAR;
            } else {
                // Mild winter
                if (roll < 0.20f) return WeatherType::RAIN_LIGHT;
                if (roll < 0.30f) return WeatherType::SNOW_LIGHT;
                if (roll < 0.45f) return WeatherType::OVERCAST;
                if (roll < 0.55f) return WeatherType::FOG;
                return WeatherType::PARTLY_CLOUDY;
            }

        default:
            return WeatherType::CLEAR;
    }
}

void WeatherSystem::updateWeatherState(float deltaTime) {
    // Apply current weather effects that change over time
    // (Most effects are handled in updateLightning and updateGroundWetness)
}

void WeatherSystem::updateTransition(float deltaTime) {
    if (!transition.isTransitioning) return;

    transition.progress += deltaTime / transition.duration;

    if (transition.progress >= 1.0f) {
        transition.progress = 1.0f;
        transition.isTransitioning = false;
        currentState = targetState;
    } else {
        currentState = interpolateStates(
            createStateForWeather(transition.fromWeather),
            targetState,
            transition.progress
        );
    }
}

void WeatherSystem::updateLightning(float deltaTime) {
    hasRecentLightning = false;

    if (currentState.type != WeatherType::THUNDERSTORM) {
        currentState.lightningIntensity = 0.0f;
        return;
    }

    lightningTimer -= deltaTime;

    if (lightningTimer <= 0.0f) {
        // Lightning flash!
        hasRecentLightning = true;
        currentState.lightningIntensity = 1.0f;

        // Random position in sky
        std::uniform_real_distribution<float> posDist(-100.0f, 100.0f);
        std::uniform_real_distribution<float> heightDist(50.0f, 150.0f);
        lastLightningPos = {posDist(weatherRng), heightDist(weatherRng), posDist(weatherRng)};

        // Next lightning in 2-10 seconds
        std::uniform_real_distribution<float> timeDist(2.0f, 10.0f);
        lightningTimer = timeDist(weatherRng);
    } else {
        // Fade out lightning
        currentState.lightningIntensity *= std::exp(-deltaTime * 10.0f);
    }
}

void WeatherSystem::updateGroundWetness(float deltaTime) {
    if (isRaining()) {
        // Build up wetness during rain
        float wetRate = currentState.precipitationIntensity * 0.1f;
        currentState.groundWetness += wetRate * deltaTime;
        currentState.groundWetness = std::min(currentState.groundWetness, 1.0f);
    } else {
        // Dry out over time
        float dryRate = 0.02f; // ~50 seconds to fully dry
        currentState.groundWetness -= dryRate * deltaTime;
        currentState.groundWetness = std::max(currentState.groundWetness, 0.0f);
    }
}

void WeatherSystem::updateSkyColors() {
    // Sky colors are set when creating weather state
    // Could add time-of-day blending here
}

WeatherState WeatherSystem::createStateForWeather(WeatherType type) const {
    WeatherState state;
    state.type = type;

    switch (type) {
        case WeatherType::CLEAR:
            state.cloudCoverage = 0.0f;
            state.sunIntensity = 1.0f;
            state.fogDensity = 0.0f;
            state.windStrength = 0.1f;
            state.skyTopColor = {0.35f, 0.55f, 0.9f};
            state.skyHorizonColor = {0.7f, 0.8f, 0.95f};
            break;

        case WeatherType::PARTLY_CLOUDY:
            state.cloudCoverage = 0.4f;
            state.sunIntensity = 0.85f;
            state.fogDensity = 0.0f;
            state.windStrength = 0.2f;
            state.skyTopColor = {0.4f, 0.55f, 0.8f};
            state.skyHorizonColor = {0.65f, 0.72f, 0.85f};
            break;

        case WeatherType::OVERCAST:
            state.cloudCoverage = 1.0f;
            state.sunIntensity = 0.4f;
            state.fogDensity = 0.1f;
            state.windStrength = 0.15f;
            state.skyTopColor = {0.5f, 0.52f, 0.55f};
            state.skyHorizonColor = {0.6f, 0.62f, 0.65f};
            break;

        case WeatherType::RAIN_LIGHT:
            state.cloudCoverage = 0.9f;
            state.precipitationIntensity = 0.3f;
            state.precipitationType = 0.0f; // Rain
            state.sunIntensity = 0.35f;
            state.fogDensity = 0.15f;
            state.windStrength = 0.25f;
            state.skyTopColor = {0.45f, 0.48f, 0.52f};
            state.skyHorizonColor = {0.55f, 0.58f, 0.62f};
            break;

        case WeatherType::RAIN_HEAVY:
            state.cloudCoverage = 1.0f;
            state.precipitationIntensity = 0.8f;
            state.precipitationType = 0.0f;
            state.sunIntensity = 0.2f;
            state.fogDensity = 0.3f;
            state.windStrength = 0.5f;
            state.skyTopColor = {0.35f, 0.38f, 0.42f};
            state.skyHorizonColor = {0.45f, 0.48f, 0.52f};
            break;

        case WeatherType::THUNDERSTORM:
            state.cloudCoverage = 1.0f;
            state.precipitationIntensity = 0.9f;
            state.precipitationType = 0.0f;
            state.sunIntensity = 0.15f;
            state.fogDensity = 0.35f;
            state.windStrength = 0.7f;
            state.skyTopColor = {0.25f, 0.28f, 0.35f};
            state.skyHorizonColor = {0.35f, 0.38f, 0.45f};
            break;

        case WeatherType::SNOW_LIGHT:
            state.cloudCoverage = 0.8f;
            state.precipitationIntensity = 0.3f;
            state.precipitationType = 1.0f; // Snow
            state.sunIntensity = 0.5f;
            state.fogDensity = 0.1f;
            state.windStrength = 0.15f;
            state.temperatureModifier = -10.0f;
            state.skyTopColor = {0.6f, 0.65f, 0.7f};
            state.skyHorizonColor = {0.75f, 0.78f, 0.82f};
            break;

        case WeatherType::SNOW_HEAVY:
            state.cloudCoverage = 1.0f;
            state.precipitationIntensity = 0.85f;
            state.precipitationType = 1.0f;
            state.sunIntensity = 0.25f;
            state.fogDensity = 0.5f;
            state.windStrength = 0.6f;
            state.temperatureModifier = -15.0f;
            state.skyTopColor = {0.55f, 0.58f, 0.62f};
            state.skyHorizonColor = {0.65f, 0.68f, 0.72f};
            break;

        case WeatherType::FOG:
            state.cloudCoverage = 0.3f;
            state.sunIntensity = 0.45f;
            state.fogDensity = 0.8f;
            state.fogHeight = 20.0f;
            state.windStrength = 0.05f;
            state.skyTopColor = {0.6f, 0.65f, 0.7f};
            state.skyHorizonColor = {0.7f, 0.75f, 0.8f};
            break;

        case WeatherType::MIST:
            state.cloudCoverage = 0.2f;
            state.sunIntensity = 0.6f;
            state.fogDensity = 0.4f;
            state.fogHeight = 10.0f;
            state.windStrength = 0.1f;
            state.skyTopColor = {0.5f, 0.6f, 0.75f};
            state.skyHorizonColor = {0.7f, 0.75f, 0.85f};
            break;

        case WeatherType::SANDSTORM:
            state.cloudCoverage = 0.1f;
            state.sunIntensity = 0.35f;
            state.fogDensity = 0.7f;
            state.windStrength = 0.9f;
            state.temperatureModifier = 5.0f;
            state.skyTopColor = {0.75f, 0.6f, 0.4f};
            state.skyHorizonColor = {0.85f, 0.7f, 0.5f};
            break;

        case WeatherType::WINDY:
            state.cloudCoverage = 0.3f;
            state.sunIntensity = 0.9f;
            state.fogDensity = 0.0f;
            state.windStrength = 0.8f;
            state.skyTopColor = {0.4f, 0.55f, 0.85f};
            state.skyHorizonColor = {0.65f, 0.75f, 0.9f};
            break;

        default:
            break;
    }

    // Set wind direction based on climate system or random
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    float angle = angleDist(weatherRng);
    state.windDirection = {std::cos(angle), std::sin(angle)};

    return state;
}

WeatherState WeatherSystem::interpolateStates(const WeatherState& a, const WeatherState& b, float t) {
    WeatherState result;

    // Keep target weather type after halfway point
    result.type = (t < 0.5f) ? a.type : b.type;

    // Linear interpolation for most values
    result.cloudCoverage = a.cloudCoverage + (b.cloudCoverage - a.cloudCoverage) * t;
    result.precipitationIntensity = a.precipitationIntensity + (b.precipitationIntensity - a.precipitationIntensity) * t;
    result.precipitationType = a.precipitationType + (b.precipitationType - a.precipitationType) * t;
    result.windStrength = a.windStrength + (b.windStrength - a.windStrength) * t;
    result.fogDensity = a.fogDensity + (b.fogDensity - a.fogDensity) * t;
    result.fogHeight = a.fogHeight + (b.fogHeight - a.fogHeight) * t;
    result.sunIntensity = a.sunIntensity + (b.sunIntensity - a.sunIntensity) * t;
    result.temperatureModifier = a.temperatureModifier + (b.temperatureModifier - a.temperatureModifier) * t;
    result.groundWetness = a.groundWetness + (b.groundWetness - a.groundWetness) * t;

    // Interpolate colors
    result.skyTopColor = a.skyTopColor + (b.skyTopColor - a.skyTopColor) * t;
    result.skyHorizonColor = a.skyHorizonColor + (b.skyHorizonColor - a.skyHorizonColor) * t;

    // Interpolate wind direction (using angle)
    float angleA = std::atan2(a.windDirection.y, a.windDirection.x);
    float angleB = std::atan2(b.windDirection.y, b.windDirection.x);

    // Handle angle wrapping
    float angleDiff = angleB - angleA;
    if (angleDiff > 3.14159f) angleDiff -= 6.28318f;
    if (angleDiff < -3.14159f) angleDiff += 6.28318f;

    float interpAngle = angleA + angleDiff * t;
    result.windDirection = {std::cos(interpAngle), std::sin(interpAngle)};

    return result;
}
