#pragma once

#include <glm/glm.hpp>
#include <string>
#include <functional>

// Forward declarations
class SeasonManager;
class ClimateSystem;

// Weather types from calm to severe
enum class WeatherType {
    CLEAR,              // Sunny, no clouds
    PARTLY_CLOUDY,      // Some clouds, still sunny
    OVERCAST,           // Full cloud cover
    RAIN_LIGHT,         // Drizzle
    RAIN_HEAVY,         // Downpour
    THUNDERSTORM,       // Heavy rain with lightning
    SNOW_LIGHT,         // Light snowfall
    SNOW_HEAVY,         // Blizzard conditions
    FOG,                // Ground fog
    MIST,               // Light fog
    SANDSTORM,          // Desert weather
    WINDY,              // Strong winds, no precipitation

    COUNT
};

// Weather data for rendering and gameplay
struct WeatherState {
    WeatherType type = WeatherType::CLEAR;

    // Cloud coverage (0 = clear, 1 = overcast)
    float cloudCoverage = 0.0f;

    // Precipitation intensity (0 = none, 1 = maximum)
    float precipitationIntensity = 0.0f;

    // Precipitation type (for shader: 0 = rain, 1 = snow)
    float precipitationType = 0.0f;

    // Wind
    glm::vec2 windDirection = {1.0f, 0.0f};
    float windStrength = 0.0f;          // 0-1

    // Fog/visibility
    float fogDensity = 0.0f;            // 0-1
    float fogHeight = 10.0f;            // Height above ground

    // Lightning (for thunderstorms)
    float lightningIntensity = 0.0f;    // Flash brightness
    float nextLightningTime = 0.0f;     // Countdown to next flash

    // Temperature modifier (affects gameplay)
    float temperatureModifier = 0.0f;   // Added to base temperature

    // Wetness (affects terrain appearance)
    float groundWetness = 0.0f;         // 0-1, builds up during rain

    // Sky colors for this weather
    glm::vec3 skyTopColor = {0.4f, 0.6f, 0.9f};
    glm::vec3 skyHorizonColor = {0.7f, 0.8f, 0.95f};
    float sunIntensity = 1.0f;
};

// Weather transition information
struct WeatherTransition {
    WeatherType fromWeather;
    WeatherType toWeather;
    float progress = 0.0f;          // 0 = start, 1 = complete
    float duration = 30.0f;         // Seconds for transition
    bool isTransitioning = false;
};

class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem() = default;

    // Initialize with references to other systems
    void initialize(SeasonManager* season, ClimateSystem* climate);

    // Update weather simulation (call each frame)
    void update(float deltaTime);

    // Get current weather state for rendering/gameplay
    const WeatherState& getCurrentWeather() const { return currentState; }

    // Get interpolated weather state (for smooth transitions)
    WeatherState getInterpolatedWeather() const;

    // Force weather change (for debugging/testing)
    void setWeather(WeatherType type, float transitionDuration = 30.0f);

    // Get weather type
    WeatherType getWeatherType() const { return currentState.type; }

    // Weather queries
    bool isRaining() const;
    bool isSnowing() const;
    bool isPrecipitating() const;
    bool isFoggy() const;
    bool isStormy() const;

    // Get weather-appropriate values
    float getVisibility() const;          // Distance in world units
    float getSunIntensity() const;        // 0-1 multiplier
    float getWindStrength() const;        // 0-1

    // Lightning effects (for thunderstorms)
    bool hasLightningFlash() const;
    glm::vec3 getLightningPosition() const;

    // Weather name for UI
    static const char* getWeatherName(WeatherType type);

    // Configuration
    void setWeatherChangeInterval(float seconds) { weatherChangeInterval = seconds; }
    void setAutoWeatherChange(bool enabled) { autoWeatherChange = enabled; }

    // Events
    using WeatherChangeCallback = std::function<void(WeatherType newWeather)>;
    void setWeatherChangeCallback(WeatherChangeCallback callback) { onWeatherChange = callback; }

private:
    SeasonManager* seasonManager = nullptr;
    ClimateSystem* climateSystem = nullptr;

    WeatherState currentState;
    WeatherState targetState;
    WeatherTransition transition;

    // Timing
    float weatherTimer = 0.0f;
    float weatherChangeInterval = 300.0f;  // 5 minutes default
    bool autoWeatherChange = true;

    // Lightning
    float lightningTimer = 0.0f;
    glm::vec3 lastLightningPos = {0, 0, 0};
    bool hasRecentLightning = false;

    // Callbacks
    WeatherChangeCallback onWeatherChange;

    // Weather determination
    WeatherType determineWeatherForConditions() const;
    WeatherType getRandomWeatherForSeason() const;
    float getWeatherProbability(WeatherType type) const;

    // State updates
    void updateWeatherState(float deltaTime);
    void updateTransition(float deltaTime);
    void updateLightning(float deltaTime);
    void updateGroundWetness(float deltaTime);
    void updateSkyColors();

    // Create target state for weather type
    WeatherState createStateForWeather(WeatherType type) const;

    // Interpolate between two weather states
    static WeatherState interpolateStates(const WeatherState& a, const WeatherState& b, float t);
};
