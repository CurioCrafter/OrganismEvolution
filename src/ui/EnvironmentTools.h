#pragma once

// EnvironmentTools - Environmental manipulation tools for God Mode
// Provides climate zones, weather control, and vegetation management

#include "imgui.h"
#include "../environment/ClimateSystem.h"
#include "../environment/WeatherSystem.h"
#include "../environment/VegetationManager.h"
#include "../graphics/Camera.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace ui {

// Temperature zone for creating hot/cold areas
struct TemperatureZone {
    glm::vec3 center{0.0f};
    float radius = 50.0f;
    float temperatureOffset = 0.0f;  // Celsius offset from ambient
    float duration = -1.0f;          // -1 = permanent
    float timeRemaining = 0.0f;
    bool active = true;
    std::string name;
};

// Food spawn point
struct FoodSpawnPoint {
    glm::vec3 position{0.0f};
    float radius = 10.0f;
    int amount = 10;
    float respawnTime = 30.0f;
    float timer = 0.0f;
    bool active = true;
};

class EnvironmentTools {
public:
    EnvironmentTools();
    ~EnvironmentTools() = default;

    // Main UI render
    void renderUI();

    // Render as section
    void renderSection();

    // Update (call each frame to update zones)
    void update(float deltaTime);

    // Set dependencies
    void setClimateSystem(ClimateSystem* climate) { m_climate = climate; }
    void setWeatherSystem(WeatherSystem* weather) { m_weather = weather; }
    void setVegetationManager(VegetationManager* vegetation) { m_vegetation = vegetation; }
    void setCamera(const Camera* camera) { m_camera = camera; }

    // Update brush position
    void updateBrush(float screenWidth, float screenHeight);

    // Temperature zone operations
    void createHotZone(const glm::vec3& center, float radius, float temperature);
    void createColdZone(const glm::vec3& center, float radius, float temperature);
    void removeZone(int index);
    void clearAllZones();

    // Weather control
    void setWeather(WeatherType type);
    void triggerWeatherEvent();

    // Climate events
    void triggerClimateEvent(ClimateEvent event, float duration);

    // Vegetation/food spawning
    void spawnFood(const glm::vec3& position, float radius, int amount);
    void createFoodSource(const glm::vec3& position, float radius, int amount, float respawnTime);
    void removeFoodSource(int index);

    // Get zones for external use (e.g., rendering)
    const std::vector<TemperatureZone>& getTemperatureZones() const { return m_temperatureZones; }
    const std::vector<FoodSpawnPoint>& getFoodSources() const { return m_foodSources; }

    // Get temperature modification at position
    float getTemperatureModifier(const glm::vec3& position) const;

    // Brush
    bool isBrushActive() const { return m_brushActive; }
    glm::vec3 getBrushPosition() const { return m_brushPosition; }

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggleVisible() { m_visible = !m_visible; }

    // Render zone visualizations
    void renderZoneVisuals(float screenWidth, float screenHeight);

private:
    // Dependencies
    ClimateSystem* m_climate = nullptr;
    WeatherSystem* m_weather = nullptr;
    VegetationManager* m_vegetation = nullptr;
    const Camera* m_camera = nullptr;

    // Temperature zones
    std::vector<TemperatureZone> m_temperatureZones;
    int m_maxZones = 20;

    // Food sources
    std::vector<FoodSpawnPoint> m_foodSources;
    int m_maxFoodSources = 50;

    // Brush state
    glm::vec3 m_brushPosition{0.0f};
    bool m_brushActive = false;

    // UI state
    bool m_visible = true;
    float m_zoneRadius = 50.0f;
    float m_zoneTemperature = 10.0f;
    bool m_zonePermanent = true;
    float m_zoneDuration = 60.0f;

    // Food settings
    int m_foodAmount = 20;
    float m_foodRadius = 15.0f;
    float m_foodRespawnTime = 30.0f;

    // Weather preset index
    int m_selectedWeatherIndex = 0;

    // Screen dimensions
    float m_screenWidth = 1920.0f;
    float m_screenHeight = 1080.0f;

    // Helper methods
    void renderWeatherControls();
    void renderClimateEventControls();
    void renderTemperatureZoneControls();
    void renderFoodControls();
    void renderActiveZonesList();
    void renderActiveFoodSourcesList();

    bool getWorldPosition(float screenX, float screenY, glm::vec3& outPos);

    // Weather type helpers
    static const char* getWeatherName(WeatherType type);
    static const char* getClimateEventName(ClimateEvent event);
};

// ============================================================================
// Implementation
// ============================================================================

inline const char* EnvironmentTools::getWeatherName(WeatherType type) {
    switch (type) {
        case WeatherType::CLEAR: return "Clear";
        case WeatherType::PARTLY_CLOUDY: return "Partly Cloudy";
        case WeatherType::OVERCAST: return "Overcast";
        case WeatherType::RAIN_LIGHT: return "Light Rain";
        case WeatherType::RAIN_HEAVY: return "Heavy Rain";
        case WeatherType::THUNDERSTORM: return "Thunderstorm";
        case WeatherType::SNOW_LIGHT: return "Light Snow";
        case WeatherType::SNOW_HEAVY: return "Heavy Snow";
        case WeatherType::FOG: return "Fog";
        case WeatherType::SANDSTORM: return "Sandstorm";
        case WeatherType::WINDY: return "Windy";
        default: return "Unknown";
    }
}

inline const char* EnvironmentTools::getClimateEventName(ClimateEvent event) {
    switch (event) {
        case ClimateEvent::NONE: return "None";
        case ClimateEvent::VOLCANIC_WINTER: return "Volcanic Winter";
        case ClimateEvent::SOLAR_MAXIMUM: return "Solar Maximum";
        case ClimateEvent::DROUGHT: return "Drought";
        case ClimateEvent::MONSOON: return "Monsoon";
        case ClimateEvent::ICE_AGE_START: return "Ice Age Start";
        case ClimateEvent::ICE_AGE_END: return "Ice Age End";
        default: return "Unknown";
    }
}

inline EnvironmentTools::EnvironmentTools() {
    m_temperatureZones.reserve(m_maxZones);
    m_foodSources.reserve(m_maxFoodSources);
}

inline bool EnvironmentTools::getWorldPosition(float screenX, float screenY, glm::vec3& outPos) {
    if (!m_camera) return false;

    float x = (2.0f * screenX) / m_screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / m_screenHeight;

    glm::mat4 view = const_cast<Camera*>(m_camera)->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix(m_screenWidth / m_screenHeight);
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 nearPoint(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint(x, y, 1.0f, 1.0f);

    glm::vec4 nearWorld = invVP * nearPoint;
    glm::vec4 farWorld = invVP * farPoint;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    glm::vec3 rayOrigin(nearWorld);
    glm::vec3 rayDir = glm::normalize(glm::vec3(farWorld) - glm::vec3(nearWorld));

    // Simple ground plane intersection at y=0
    if (abs(rayDir.y) > 0.001f) {
        float t = -rayOrigin.y / rayDir.y;
        if (t > 0) {
            outPos = rayOrigin + rayDir * t;
            return true;
        }
    }

    return false;
}

inline void EnvironmentTools::updateBrush(float screenWidth, float screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        m_brushActive = false;
        return;
    }

    m_brushActive = getWorldPosition(io.MousePos.x, io.MousePos.y, m_brushPosition);
}

inline void EnvironmentTools::update(float deltaTime) {
    // Update temperature zones with duration
    for (auto& zone : m_temperatureZones) {
        if (zone.duration > 0 && zone.active) {
            zone.timeRemaining -= deltaTime;
            if (zone.timeRemaining <= 0) {
                zone.active = false;
            }
        }
    }

    // Remove inactive zones
    m_temperatureZones.erase(
        std::remove_if(m_temperatureZones.begin(), m_temperatureZones.end(),
            [](const TemperatureZone& z) { return !z.active && z.duration > 0; }),
        m_temperatureZones.end()
    );

    // Update food sources (respawning)
    for (auto& source : m_foodSources) {
        if (source.active) {
            source.timer += deltaTime;
            if (source.timer >= source.respawnTime) {
                source.timer = 0.0f;
                // Would spawn food here if vegetation manager supported it
            }
        }
    }
}

inline void EnvironmentTools::createHotZone(const glm::vec3& center, float radius, float temperature) {
    if (static_cast<int>(m_temperatureZones.size()) >= m_maxZones) {
        // Remove oldest zone
        m_temperatureZones.erase(m_temperatureZones.begin());
    }

    TemperatureZone zone;
    zone.center = center;
    zone.radius = radius;
    zone.temperatureOffset = abs(temperature);  // Positive = hot
    zone.duration = m_zonePermanent ? -1.0f : m_zoneDuration;
    zone.timeRemaining = zone.duration;
    zone.name = "Hot Zone " + std::to_string(m_temperatureZones.size() + 1);
    zone.active = true;

    m_temperatureZones.push_back(zone);
}

inline void EnvironmentTools::createColdZone(const glm::vec3& center, float radius, float temperature) {
    if (static_cast<int>(m_temperatureZones.size()) >= m_maxZones) {
        m_temperatureZones.erase(m_temperatureZones.begin());
    }

    TemperatureZone zone;
    zone.center = center;
    zone.radius = radius;
    zone.temperatureOffset = -abs(temperature);  // Negative = cold
    zone.duration = m_zonePermanent ? -1.0f : m_zoneDuration;
    zone.timeRemaining = zone.duration;
    zone.name = "Cold Zone " + std::to_string(m_temperatureZones.size() + 1);
    zone.active = true;

    m_temperatureZones.push_back(zone);
}

inline void EnvironmentTools::removeZone(int index) {
    if (index >= 0 && index < static_cast<int>(m_temperatureZones.size())) {
        m_temperatureZones.erase(m_temperatureZones.begin() + index);
    }
}

inline void EnvironmentTools::clearAllZones() {
    m_temperatureZones.clear();
}

inline float EnvironmentTools::getTemperatureModifier(const glm::vec3& position) const {
    float totalModifier = 0.0f;

    for (const auto& zone : m_temperatureZones) {
        if (!zone.active) continue;

        float dist = glm::length(glm::vec2(position.x - zone.center.x,
                                           position.z - zone.center.z));

        if (dist < zone.radius) {
            // Linear falloff from center
            float influence = 1.0f - (dist / zone.radius);
            totalModifier += zone.temperatureOffset * influence;
        }
    }

    return totalModifier;
}

inline void EnvironmentTools::setWeather(WeatherType type) {
    if (m_weather) {
        m_weather->setWeather(type, 10.0f);  // 10 second transition
    }
}

inline void EnvironmentTools::triggerWeatherEvent() {
    // Cycle through weather types or trigger random event
    if (m_weather) {
        // Get random weather
        int weatherIdx = rand() % static_cast<int>(WeatherType::WINDY);
        m_weather->setWeather(static_cast<WeatherType>(weatherIdx), 5.0f);
    }
}

inline void EnvironmentTools::triggerClimateEvent(ClimateEvent event, float duration) {
    if (m_climate) {
        m_climate->startEvent(event, duration);
    }
}

inline void EnvironmentTools::spawnFood(const glm::vec3& position, float radius, int amount) {
    // This would interact with ecosystem/vegetation manager
    // Currently a placeholder - would need VegetationManager to expose food spawning
}

inline void EnvironmentTools::createFoodSource(const glm::vec3& position, float radius,
                                                 int amount, float respawnTime) {
    if (static_cast<int>(m_foodSources.size()) >= m_maxFoodSources) {
        m_foodSources.erase(m_foodSources.begin());
    }

    FoodSpawnPoint source;
    source.position = position;
    source.radius = radius;
    source.amount = amount;
    source.respawnTime = respawnTime;
    source.timer = 0.0f;
    source.active = true;

    m_foodSources.push_back(source);
}

inline void EnvironmentTools::removeFoodSource(int index) {
    if (index >= 0 && index < static_cast<int>(m_foodSources.size())) {
        m_foodSources.erase(m_foodSources.begin() + index);
    }
}

inline void EnvironmentTools::renderUI() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(380, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Environment Tools", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void EnvironmentTools::renderSection() {
    // Weather controls
    if (ImGui::CollapsingHeader("Weather Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderWeatherControls();
    }

    // Climate events
    if (ImGui::CollapsingHeader("Climate Events")) {
        renderClimateEventControls();
    }

    // Temperature zones
    if (ImGui::CollapsingHeader("Temperature Zones", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTemperatureZoneControls();
    }

    // Food spawning
    if (ImGui::CollapsingHeader("Food Sources")) {
        renderFoodControls();
    }

    // Active zones list
    if (!m_temperatureZones.empty() && ImGui::CollapsingHeader("Active Zones")) {
        renderActiveZonesList();
    }

    // Active food sources list
    if (!m_foodSources.empty() && ImGui::CollapsingHeader("Active Food Sources")) {
        renderActiveFoodSourcesList();
    }
}

inline void EnvironmentTools::renderWeatherControls() {
    // Current weather display
    if (m_weather) {
        const auto& state = m_weather->getCurrentWeather();
        ImGui::Text("Current Weather: %s", getWeatherName(state.type));
        ImGui::Text("Cloud Cover: %.0f%%", state.cloudCoverage * 100.0f);
        ImGui::Text("Precipitation: %.2f", state.precipitationIntensity);
        ImGui::Text("Wind: %.1f", state.windSpeed);

        ImGui::Separator();
    }

    // Weather selection
    ImGui::Text("Set Weather:");

    // Weather buttons in grid
    const WeatherType weathers[] = {
        WeatherType::CLEAR, WeatherType::PARTLY_CLOUDY, WeatherType::OVERCAST,
        WeatherType::RAIN_LIGHT, WeatherType::RAIN_HEAVY, WeatherType::THUNDERSTORM,
        WeatherType::FOG, WeatherType::SNOW_LIGHT, WeatherType::WINDY
    };

    for (int i = 0; i < 9; ++i) {
        if (i > 0 && i % 3 != 0) ImGui::SameLine();

        if (ImGui::Button(getWeatherName(weathers[i]), ImVec2(110, 0))) {
            setWeather(weathers[i]);
        }
    }

    ImGui::Separator();

    // Random weather button
    if (ImGui::Button("Random Weather", ImVec2(-1, 0))) {
        triggerWeatherEvent();
    }
}

inline void EnvironmentTools::renderClimateEventControls() {
    // Current event display
    if (m_climate) {
        if (m_climate->hasActiveEvent()) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                "Active Event: %s", m_climate->getEventName());
            ImGui::Text("Time Remaining: %.1f s", m_climate->getEventTimeRemaining());
            ImGui::Separator();
        }

        ImGui::Text("Global Temperature: %.1f C", m_climate->getGlobalTemperature());
        ImGui::Separator();
    }

    // Event buttons
    ImGui::Text("Trigger Event:");

    if (ImGui::Button("Volcanic Winter", ImVec2(140, 25))) {
        triggerClimateEvent(ClimateEvent::VOLCANIC_WINTER, 120.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Solar Maximum", ImVec2(140, 25))) {
        triggerClimateEvent(ClimateEvent::SOLAR_MAXIMUM, 120.0f);
    }

    if (ImGui::Button("Drought", ImVec2(140, 25))) {
        triggerClimateEvent(ClimateEvent::DROUGHT, 90.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Monsoon", ImVec2(140, 25))) {
        triggerClimateEvent(ClimateEvent::MONSOON, 60.0f);
    }

    ImGui::Separator();

    // Ice age controls
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::Button("Start Ice Age", ImVec2(140, 25))) {
        triggerClimateEvent(ClimateEvent::ICE_AGE_START, 300.0f);
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.3f, 1.0f));
    if (ImGui::Button("End Ice Age", ImVec2(140, 25))) {
        triggerClimateEvent(ClimateEvent::ICE_AGE_END, 300.0f);
    }
    ImGui::PopStyleColor();

    // End current event
    if (m_climate && m_climate->hasActiveEvent()) {
        ImGui::Separator();
        if (ImGui::Button("End Current Event", ImVec2(-1, 0))) {
            m_climate->endEvent();
        }
    }
}

inline void EnvironmentTools::renderTemperatureZoneControls() {
    // Zone settings
    ImGui::SliderFloat("Zone Radius", &m_zoneRadius, 10.0f, 200.0f, "%.0f");
    ImGui::SliderFloat("Temperature", &m_zoneTemperature, 1.0f, 30.0f, "%.1f C");

    ImGui::Checkbox("Permanent", &m_zonePermanent);
    if (!m_zonePermanent) {
        ImGui::SliderFloat("Duration", &m_zoneDuration, 10.0f, 300.0f, "%.0f s");
    }

    ImGui::Separator();

    // Create zone buttons
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.2f, 1.0f));
    if (ImGui::Button("Create HOT Zone", ImVec2(-1, 30))) {
        if (m_brushActive) {
            createHotZone(m_brushPosition, m_zoneRadius, m_zoneTemperature);
        }
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
    if (ImGui::Button("Create COLD Zone", ImVec2(-1, 30))) {
        if (m_brushActive) {
            createColdZone(m_brushPosition, m_zoneRadius, m_zoneTemperature);
        }
    }
    ImGui::PopStyleColor();

    // Brush position
    if (m_brushActive) {
        ImGui::Text("Brush: (%.1f, %.1f)", m_brushPosition.x, m_brushPosition.z);
    } else {
        ImGui::TextDisabled("Move mouse to position zone");
    }

    // Clear all
    if (!m_temperatureZones.empty()) {
        ImGui::Separator();
        if (ImGui::Button("Clear All Zones", ImVec2(-1, 0))) {
            clearAllZones();
        }
    }
}

inline void EnvironmentTools::renderFoodControls() {
    ImGui::SliderInt("Food Amount", &m_foodAmount, 1, 100);
    ImGui::SliderFloat("Spawn Radius", &m_foodRadius, 5.0f, 50.0f, "%.0f");
    ImGui::SliderFloat("Respawn Time", &m_foodRespawnTime, 5.0f, 120.0f, "%.0f s");

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
    if (ImGui::Button("Spawn Food Once", ImVec2(-1, 25))) {
        if (m_brushActive) {
            spawnFood(m_brushPosition, m_foodRadius, m_foodAmount);
        }
    }
    ImGui::PopStyleColor();

    if (ImGui::Button("Create Food Source", ImVec2(-1, 25))) {
        if (m_brushActive) {
            createFoodSource(m_brushPosition, m_foodRadius, m_foodAmount, m_foodRespawnTime);
        }
    }

    if (m_brushActive) {
        ImGui::Text("Brush: (%.1f, %.1f)", m_brushPosition.x, m_brushPosition.z);
    } else {
        ImGui::TextDisabled("Move mouse to position food");
    }
}

inline void EnvironmentTools::renderActiveZonesList() {
    ImGui::Text("Active Zones: %d", static_cast<int>(m_temperatureZones.size()));

    for (int i = 0; i < static_cast<int>(m_temperatureZones.size()); ++i) {
        auto& zone = m_temperatureZones[i];

        ImGui::PushID(i);

        // Zone color based on type
        ImVec4 color = zone.temperatureOffset > 0
            ? ImVec4(0.8f, 0.3f, 0.2f, 1.0f)
            : ImVec4(0.2f, 0.4f, 0.8f, 1.0f);

        ImGui::TextColored(color, "%s", zone.name.c_str());
        ImGui::SameLine(200);
        ImGui::Text("%.1f C", zone.temperatureOffset);
        ImGui::SameLine(270);

        if (zone.duration > 0) {
            ImGui::Text("%.0fs", zone.timeRemaining);
        } else {
            ImGui::TextDisabled("Perm");
        }

        ImGui::SameLine(320);
        if (ImGui::SmallButton("X")) {
            removeZone(i);
            ImGui::PopID();
            break;
        }

        ImGui::PopID();
    }
}

inline void EnvironmentTools::renderActiveFoodSourcesList() {
    ImGui::Text("Food Sources: %d", static_cast<int>(m_foodSources.size()));

    for (int i = 0; i < static_cast<int>(m_foodSources.size()); ++i) {
        auto& source = m_foodSources[i];

        ImGui::PushID(i + 1000);

        ImGui::Text("Source %d", i + 1);
        ImGui::SameLine(80);
        ImGui::Text("x%d", source.amount);
        ImGui::SameLine(130);
        ImGui::Text("%.0fs", source.respawnTime - source.timer);
        ImGui::SameLine(180);

        if (ImGui::SmallButton("X")) {
            removeFoodSource(i);
            ImGui::PopID();
            break;
        }

        ImGui::PopID();
    }
}

inline void EnvironmentTools::renderZoneVisuals(float screenWidth, float screenHeight) {
    if (!m_camera || m_temperatureZones.empty()) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    glm::mat4 view = const_cast<Camera*>(m_camera)->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix(screenWidth / screenHeight);
    glm::mat4 vp = proj * view;

    for (const auto& zone : m_temperatureZones) {
        if (!zone.active) continue;

        // Project center to screen
        glm::vec4 clipPos = vp * glm::vec4(zone.center, 1.0f);
        if (clipPos.w <= 0) continue;

        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;

        // Approximate screen radius
        float radiusOnScreen = zone.radius * 3.0f / clipPos.w;
        radiusOnScreen = glm::clamp(radiusOnScreen, 20.0f, 300.0f);

        // Zone color
        ImU32 fillColor = zone.temperatureOffset > 0
            ? IM_COL32(255, 100, 50, 30)
            : IM_COL32(50, 100, 255, 30);

        ImU32 borderColor = zone.temperatureOffset > 0
            ? IM_COL32(255, 100, 50, 150)
            : IM_COL32(50, 100, 255, 150);

        // Draw zone
        drawList->AddCircleFilled(ImVec2(screenX, screenY), radiusOnScreen, fillColor, 32);
        drawList->AddCircle(ImVec2(screenX, screenY), radiusOnScreen, borderColor, 32, 2.0f);

        // Label
        char label[32];
        snprintf(label, sizeof(label), "%+.0f C", zone.temperatureOffset);
        drawList->AddText(ImVec2(screenX - 15, screenY - 8), borderColor, label);
    }
}

} // namespace ui
