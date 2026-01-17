#pragma once

#include <string>

enum class Season {
    SPRING,  // Growth, reproduction boost
    SUMMER,  // Peak abundance
    FALL,    // Harvest, fruits available
    WINTER   // Scarcity, dormancy
};

class SeasonManager {
public:
    SeasonManager();

    void update(float deltaTime);

    // Current state
    Season getCurrentSeason() const { return currentSeason; }
    float getSeasonProgress() const { return seasonProgress; }
    int getCurrentDay() const { return currentDay; }
    int getCurrentYear() const { return currentYear; }

    // Growth multipliers for different resources
    float getGrowthMultiplier() const;
    float getBerryMultiplier() const;
    float getFruitMultiplier() const;
    float getLeafMultiplier() const;

    // Creature behavior multipliers
    float getHerbivoreReproductionMultiplier() const;
    float getCarnivoreActivityMultiplier() const;
    float getMetabolismMultiplier() const;
    float getDecompositionMultiplier() const;

    // Visual/environmental
    float getDayLength() const;  // Hours of daylight
    float getTemperature() const; // Normalized 0-1

    // Utilities
    const char* getSeasonName() const;
    std::string getDateString() const;

    // Configuration
    void setDayDuration(float realSeconds) { dayDuration = realSeconds; }
    float getDayDuration() const { return dayDuration; }

private:
    Season currentSeason;
    float seasonProgress;    // 0-1 progress through current season
    float totalTime;         // Total elapsed time
    int currentDay;          // Day of year (0-359)
    int currentYear;

    // Timing configuration
    float dayDuration;       // Real seconds per game day (default 60)
    static constexpr int daysPerSeason = 90;
    static constexpr int daysPerYear = 360;

    void updateSeason();
};
