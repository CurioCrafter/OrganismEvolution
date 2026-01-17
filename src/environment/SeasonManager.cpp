#include "SeasonManager.h"
#include <cmath>
#include <sstream>
#include <iomanip>

SeasonManager::SeasonManager()
    : currentSeason(Season::SPRING)
    , seasonProgress(0.0f)
    , totalTime(0.0f)
    , currentDay(0)
    , currentYear(1)
    , dayDuration(60.0f)  // 60 real seconds = 1 game day
{
}

void SeasonManager::update(float deltaTime) {
    totalTime += deltaTime;

    // Calculate current day
    float totalDays = totalTime / dayDuration;
    currentDay = static_cast<int>(std::fmod(totalDays, static_cast<float>(daysPerYear)));
    currentYear = 1 + static_cast<int>(totalDays / daysPerYear);

    updateSeason();
}

void SeasonManager::updateSeason() {
    // Determine season based on day of year
    if (currentDay < daysPerSeason) {
        currentSeason = Season::SPRING;
        seasonProgress = static_cast<float>(currentDay) / daysPerSeason;
    }
    else if (currentDay < daysPerSeason * 2) {
        currentSeason = Season::SUMMER;
        seasonProgress = static_cast<float>(currentDay - daysPerSeason) / daysPerSeason;
    }
    else if (currentDay < daysPerSeason * 3) {
        currentSeason = Season::FALL;
        seasonProgress = static_cast<float>(currentDay - daysPerSeason * 2) / daysPerSeason;
    }
    else {
        currentSeason = Season::WINTER;
        seasonProgress = static_cast<float>(currentDay - daysPerSeason * 3) / daysPerSeason;
    }
}

float SeasonManager::getGrowthMultiplier() const {
    switch (currentSeason) {
        case Season::SPRING: return 1.5f;
        case Season::SUMMER: return 1.0f;
        case Season::FALL:   return 0.5f;
        case Season::WINTER: return 0.1f;
    }
    return 1.0f;
}

float SeasonManager::getBerryMultiplier() const {
    // Berries mainly available in summer and fall
    switch (currentSeason) {
        case Season::SPRING: return 0.2f;
        case Season::SUMMER: return 1.0f;
        case Season::FALL:   return 1.5f;
        case Season::WINTER: return 0.0f;
    }
    return 1.0f;
}

float SeasonManager::getFruitMultiplier() const {
    // Tree fruit mainly in summer and early fall
    switch (currentSeason) {
        case Season::SPRING: return 0.1f;
        case Season::SUMMER: return 1.2f;
        case Season::FALL:   return 1.0f - seasonProgress * 0.8f;  // Decreases through fall
        case Season::WINTER: return 0.0f;
    }
    return 1.0f;
}

float SeasonManager::getLeafMultiplier() const {
    // Leaves available spring through fall
    switch (currentSeason) {
        case Season::SPRING: return 0.8f + seasonProgress * 0.2f;  // Growing
        case Season::SUMMER: return 1.0f;
        case Season::FALL:   return 1.0f - seasonProgress * 0.7f;  // Falling
        case Season::WINTER: return 0.1f;  // Evergreens only
    }
    return 1.0f;
}

float SeasonManager::getHerbivoreReproductionMultiplier() const {
    // Herbivores reproduce mainly in spring
    switch (currentSeason) {
        case Season::SPRING: return 1.5f;
        case Season::SUMMER: return 1.0f;
        case Season::FALL:   return 0.5f;
        case Season::WINTER: return 0.2f;
    }
    return 1.0f;
}

float SeasonManager::getCarnivoreActivityMultiplier() const {
    // Carnivores more active in fall (fattening up) and winter (hungry)
    switch (currentSeason) {
        case Season::SPRING: return 1.0f;
        case Season::SUMMER: return 1.0f;
        case Season::FALL:   return 1.2f;
        case Season::WINTER: return 0.8f;  // Less active but more desperate
    }
    return 1.0f;
}

float SeasonManager::getMetabolismMultiplier() const {
    // Higher metabolism in cold (need more energy to stay warm)
    switch (currentSeason) {
        case Season::SPRING: return 1.0f;
        case Season::SUMMER: return 1.0f;
        case Season::FALL:   return 1.2f;
        case Season::WINTER: return 1.5f;
    }
    return 1.0f;
}

float SeasonManager::getDecompositionMultiplier() const {
    // Decomposition faster in warm, wet conditions
    switch (currentSeason) {
        case Season::SPRING: return 1.0f;
        case Season::SUMMER: return 1.5f;
        case Season::FALL:   return 1.0f;
        case Season::WINTER: return 0.3f;
    }
    return 1.0f;
}

float SeasonManager::getDayLength() const {
    // Hours of daylight (simplified, based on mid-latitudes)
    float baseHours = 12.0f;
    float variation = 4.0f;

    // Calculate based on day of year (longest day = summer solstice = day 180)
    float dayAngle = (currentDay - 90) * 3.14159f / 180.0f;  // Offset so spring equinox is at 0
    return baseHours + variation * std::sin(dayAngle);
}

float SeasonManager::getTemperature() const {
    // Normalized temperature 0-1 (cold to hot)
    // Peak in summer, lowest in winter
    float dayAngle = (currentDay - 90) * 3.14159f / 180.0f;
    return 0.5f + 0.4f * std::sin(dayAngle);  // Range 0.1 to 0.9
}

const char* SeasonManager::getSeasonName() const {
    switch (currentSeason) {
        case Season::SPRING: return "Spring";
        case Season::SUMMER: return "Summer";
        case Season::FALL:   return "Fall";
        case Season::WINTER: return "Winter";
    }
    return "Unknown";
}

std::string SeasonManager::getDateString() const {
    std::ostringstream ss;
    ss << getSeasonName() << " Day " << (currentDay % daysPerSeason + 1)
       << ", Year " << currentYear;
    return ss.str();
}
