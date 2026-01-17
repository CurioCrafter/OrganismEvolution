// DayNightCycle.h - Time-of-day system for dynamic lighting and sky colors
#pragma once

#include "Math/Vector.h"
#include "../entities/CreatureType.h"
#include <cmath>

using namespace Forge;
using namespace Forge::Math;

// ============================================================================
// Constants
// ============================================================================
static constexpr f32 PI_F = 3.14159265359f;
static constexpr f32 TWO_PI_F = 6.28318530718f;
static constexpr f32 LUNAR_CYCLE_DAYS = 29.5f;  // Days per lunar cycle

// ============================================================================
// Sky Colors Structure
// ============================================================================

struct SkyColors
{
    Vec3 skyTop;        // Sky color at zenith
    Vec3 skyHorizon;    // Sky color at horizon
    Vec3 sunColor;      // Sun/moon light color
    Vec3 ambientColor;  // Ambient light color
    f32 sunIntensity;   // Light intensity multiplier
};

// ============================================================================
// Color Grading Parameters (for time-of-day post-processing)
// ============================================================================
struct ColorGradingParams
{
    Vec3 shadowTint;            // Tint for shadows
    f32 shadowTintStrength;     // 0-1
    Vec3 highlightTint;         // Tint for highlights
    f32 highlightTintStrength;  // 0-1
    f32 saturation;             // 0-2 (1 = neutral)
    f32 contrast;               // 0-2 (1 = neutral)
    f32 exposure;               // Exposure adjustment
    f32 temperature;            // Color temperature shift (-1 cool to +1 warm)
};

// ============================================================================
// Day/Night Cycle System
// ============================================================================

class DayNightCycle
{
public:
    f32 dayTime = 0.25f;            // 0-1, 0=midnight, 0.5=noon, start at dawn
    f32 dayLengthSeconds = 120.0f;  // 2 minutes = full day cycle
    bool paused = false;

    // ========================================================================
    // Core Update
    // ========================================================================

    void Update(f32 dt)
    {
        if (paused) return;
        dayTime += dt / dayLengthSeconds;
        if (dayTime >= 1.0f)
        {
            dayTime -= 1.0f;
            m_dayCount += 1.0f;
            m_moonPhaseDay = std::fmod(m_moonPhaseDay + 1.0f, LUNAR_CYCLE_DAYS);
        }
        if (dayTime < 0.0f) dayTime += 1.0f;
    }

    // ========================================================================
    // Time Control
    // ========================================================================

    void SetTime(f32 normalizedTime) { dayTime = std::fmod(normalizedTime, 1.0f); }
    void SetTimeScale(f32 scale) { dayLengthSeconds = 120.0f / scale; }
    void SetDayDuration(f32 seconds) { dayLengthSeconds = seconds; }
    void SetTimeOfDay(f32 t) { dayTime = std::fmod(t, 1.0f); if (dayTime < 0) dayTime += 1.0f; }
    f32 GetTimeOfDay() const { return dayTime; }
    f32 GetNormalizedTime() const { return dayTime; }
    f32 GetDayNumber() const { return m_dayCount; }

    // ========================================================================
    // Moon Phase (0-1, 0=new moon, 0.5=full moon)
    // ========================================================================

    f32 GetMoonPhase() const
    {
        return m_moonPhaseDay / LUNAR_CYCLE_DAYS;
    }

    // Moon visibility (0 at new moon, 1 at full moon)
    f32 GetMoonVisibility() const
    {
        f32 phase = GetMoonPhase();
        // Use cosine curve: 0 at new moon (phase=0), 1 at full moon (phase=0.5)
        return 0.5f * (1.0f - std::cos(phase * TWO_PI_F));
    }

    // Time of day queries
    bool IsDay() const { return dayTime > 0.2f && dayTime < 0.8f; }
    bool IsNight() const { return !IsDay(); }
    bool IsDawn() const { return dayTime > 0.15f && dayTime < 0.3f; }
    bool IsDusk() const { return dayTime > 0.7f && dayTime < 0.85f; }
    bool IsMidnight() const { return dayTime < 0.05f || dayTime > 0.95f; }
    bool IsNoon() const { return dayTime > 0.45f && dayTime < 0.55f; }

    f32 GetSunAngle() const
    {
        // Sun rises at 0.2 (dayTime), reaches zenith at 0.5, sets at 0.8
        // Maps dayTime [0.2, 0.8] to angle [0, PI]
        return (dayTime - 0.2f) / 0.6f * 3.14159f;
    }

    Vec3 GetSunPosition() const
    {
        if (IsNight())
        {
            return GetMoonPosition();
        }

        f32 sunAngle = GetSunAngle();
        f32 sunHeight = std::sin(sunAngle);
        f32 sunHorizontal = std::cos(sunAngle);

        // Sun moves from East (+X) to West (-X)
        return Vec3(
            sunHorizontal * 500.0f,
            sunHeight * 400.0f + 50.0f,  // Minimum height of 50
            -100.0f  // Slight Z offset for interesting shadows
        );
    }

    Vec3 GetMoonPosition() const
    {
        // Moon is roughly opposite to sun position
        f32 moonPhase = dayTime + 0.5f;
        if (moonPhase >= 1.0f) moonPhase -= 1.0f;

        f32 moonAngle = moonPhase * 3.14159f * 2.0f;
        return Vec3(
            std::cos(moonAngle) * 300.0f,
            std::abs(std::sin(moonAngle)) * 200.0f + 100.0f,
            50.0f
        );
    }

    Vec3 GetLightDirection() const
    {
        Vec3 lightPos = GetSunPosition();
        return lightPos.Normalized();
    }

    SkyColors GetSkyColors() const
    {
        SkyColors colors;

        if (dayTime < 0.15f)
        {
            // Deep night (midnight to before pre-dawn)
            colors.skyTop = Vec3(0.01f, 0.01f, 0.05f);
            colors.skyHorizon = Vec3(0.03f, 0.03f, 0.08f);
            colors.sunColor = Vec3(0.2f, 0.25f, 0.4f);  // Moonlight - cool blue
            colors.ambientColor = Vec3(0.03f, 0.03f, 0.06f);
            colors.sunIntensity = 0.15f;
        }
        else if (dayTime < 0.2f)
        {
            // Pre-dawn (faint light on horizon)
            f32 t = (dayTime - 0.15f) / 0.05f;
            colors.skyTop = LerpVec3(Vec3(0.01f, 0.01f, 0.05f), Vec3(0.05f, 0.05f, 0.15f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.03f, 0.03f, 0.08f), Vec3(0.3f, 0.2f, 0.25f), t);
            colors.sunColor = LerpVec3(Vec3(0.2f, 0.25f, 0.4f), Vec3(0.6f, 0.4f, 0.3f), t);
            colors.ambientColor = LerpVec3(Vec3(0.03f, 0.03f, 0.06f), Vec3(0.1f, 0.08f, 0.08f), t);
            colors.sunIntensity = LerpF32(0.15f, 0.3f, t);
        }
        else if (dayTime < 0.3f)
        {
            // Dawn (golden hour - sunrise) - boosted ambient for better visibility
            f32 t = (dayTime - 0.2f) / 0.1f;
            colors.skyTop = LerpVec3(Vec3(0.05f, 0.05f, 0.15f), Vec3(0.4f, 0.55f, 0.85f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.3f, 0.2f, 0.25f), Vec3(0.95f, 0.6f, 0.35f), t);
            colors.sunColor = LerpVec3(Vec3(0.6f, 0.4f, 0.3f), Vec3(1.0f, 0.85f, 0.7f), t);
            colors.ambientColor = LerpVec3(Vec3(0.15f, 0.12f, 0.12f), Vec3(0.4f, 0.35f, 0.3f), t);
            colors.sunIntensity = LerpF32(0.5f, 0.9f, t);
        }
        else if (dayTime < 0.45f)
        {
            // Morning (transition to full day) - boosted for better visibility
            f32 t = (dayTime - 0.3f) / 0.15f;
            colors.skyTop = LerpVec3(Vec3(0.4f, 0.55f, 0.85f), Vec3(0.4f, 0.6f, 0.9f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.95f, 0.6f, 0.35f), Vec3(0.7f, 0.8f, 0.95f), t);
            colors.sunColor = LerpVec3(Vec3(1.0f, 0.85f, 0.7f), Vec3(1.0f, 0.98f, 0.92f), t);
            colors.ambientColor = LerpVec3(Vec3(0.4f, 0.35f, 0.3f), Vec3(0.45f, 0.5f, 0.55f), t);
            colors.sunIntensity = LerpF32(0.9f, 1.0f, t);
        }
        else if (dayTime < 0.55f)
        {
            // Midday (bright, slightly warm) - strong lighting for clear visibility
            colors.skyTop = Vec3(0.4f, 0.6f, 0.9f);
            colors.skyHorizon = Vec3(0.7f, 0.8f, 0.95f);
            colors.sunColor = Vec3(1.0f, 0.98f, 0.92f);
            colors.ambientColor = Vec3(0.45f, 0.5f, 0.55f);
            colors.sunIntensity = 1.0f;
        }
        else if (dayTime < 0.7f)
        {
            // Afternoon (transition to evening)
            f32 t = (dayTime - 0.55f) / 0.15f;
            colors.skyTop = LerpVec3(Vec3(0.4f, 0.6f, 0.9f), Vec3(0.45f, 0.5f, 0.75f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.7f, 0.8f, 0.95f), Vec3(0.85f, 0.7f, 0.5f), t);
            colors.sunColor = LerpVec3(Vec3(1.0f, 0.98f, 0.92f), Vec3(1.0f, 0.9f, 0.7f), t);
            colors.ambientColor = LerpVec3(Vec3(0.4f, 0.45f, 0.5f), Vec3(0.35f, 0.32f, 0.3f), t);
            colors.sunIntensity = LerpF32(1.0f, 0.9f, t);
        }
        else if (dayTime < 0.8f)
        {
            // Dusk (golden hour - sunset)
            f32 t = (dayTime - 0.7f) / 0.1f;
            colors.skyTop = LerpVec3(Vec3(0.45f, 0.5f, 0.75f), Vec3(0.25f, 0.15f, 0.35f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.85f, 0.7f, 0.5f), Vec3(0.98f, 0.45f, 0.2f), t);
            colors.sunColor = LerpVec3(Vec3(1.0f, 0.9f, 0.7f), Vec3(1.0f, 0.5f, 0.2f), t);
            colors.ambientColor = LerpVec3(Vec3(0.35f, 0.32f, 0.3f), Vec3(0.15f, 0.1f, 0.12f), t);
            colors.sunIntensity = LerpF32(0.9f, 0.35f, t);
        }
        else if (dayTime < 0.85f)
        {
            // Twilight (after sunset)
            f32 t = (dayTime - 0.8f) / 0.05f;
            colors.skyTop = LerpVec3(Vec3(0.25f, 0.15f, 0.35f), Vec3(0.08f, 0.05f, 0.15f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.98f, 0.45f, 0.2f), Vec3(0.2f, 0.1f, 0.15f), t);
            colors.sunColor = LerpVec3(Vec3(1.0f, 0.5f, 0.2f), Vec3(0.3f, 0.3f, 0.45f), t);
            colors.ambientColor = LerpVec3(Vec3(0.15f, 0.1f, 0.12f), Vec3(0.05f, 0.04f, 0.07f), t);
            colors.sunIntensity = LerpF32(0.35f, 0.18f, t);
        }
        else
        {
            // Night (twilight to midnight)
            f32 t = (dayTime - 0.85f) / 0.15f;
            colors.skyTop = LerpVec3(Vec3(0.08f, 0.05f, 0.15f), Vec3(0.01f, 0.01f, 0.05f), t);
            colors.skyHorizon = LerpVec3(Vec3(0.2f, 0.1f, 0.15f), Vec3(0.03f, 0.03f, 0.08f), t);
            colors.sunColor = LerpVec3(Vec3(0.3f, 0.3f, 0.45f), Vec3(0.2f, 0.25f, 0.4f), t);
            colors.ambientColor = LerpVec3(Vec3(0.05f, 0.04f, 0.07f), Vec3(0.03f, 0.03f, 0.06f), t);
            colors.sunIntensity = LerpF32(0.18f, 0.15f, t);
        }

        return colors;
    }

    // Get star visibility (0 = no stars, 1 = full stars)
    f32 GetStarVisibility() const
    {
        if (dayTime >= 0.2f && dayTime <= 0.8f) return 0.0f;  // No stars during day
        if (dayTime < 0.15f || dayTime > 0.85f) return 1.0f;  // Full stars at night

        // Fade in/out during twilight
        if (dayTime < 0.2f)
            return 1.0f - (dayTime - 0.15f) / 0.05f;
        else
            return (dayTime - 0.8f) / 0.05f;
    }

    // Get activity multiplier for creatures (lower at night)
    f32 GetCreatureActivityMultiplier(CreatureType type) const
    {
        f32 baseActivity = 1.0f;

        if (IsNight())
        {
            // Most creatures less active at night
            baseActivity = 0.5f;
        }

        // Carnivores more active at dawn/dusk (prime hunting time)
        if (type == CreatureType::CARNIVORE && (IsDawn() || IsDusk()))
        {
            baseActivity = 1.4f;
        }

        return baseActivity;
    }

    const char* GetTimeOfDayString() const
    {
        if (IsMidnight()) return "Midnight";
        if (dayTime < 0.15f) return "Night";
        if (dayTime < 0.2f) return "Pre-Dawn";
        if (dayTime < 0.3f) return "Dawn";
        if (dayTime < 0.45f) return "Morning";
        if (IsNoon()) return "Noon";
        if (dayTime < 0.7f) return "Afternoon";
        if (dayTime < 0.8f) return "Dusk";
        if (dayTime < 0.85f) return "Twilight";
        return "Night";
    }

    // ========================================================================
    // Color Grading for Post-Processing
    // ========================================================================

    ColorGradingParams GetColorGrading() const
    {
        ColorGradingParams params;
        params.shadowTint = Vec3(0.5f, 0.5f, 0.5f);
        params.shadowTintStrength = 0.0f;
        params.highlightTint = Vec3(1.0f, 1.0f, 1.0f);
        params.highlightTintStrength = 0.0f;
        params.saturation = 1.0f;
        params.contrast = 1.0f;
        params.exposure = 1.0f;
        params.temperature = 0.0f;

        if (dayTime > 0.2f && dayTime < 0.35f)
        {
            // Dawn - golden hour, warm tones
            f32 t = (dayTime - 0.2f) / 0.15f;
            params.shadowTint = Vec3(0.2f, 0.1f, 0.3f);  // Purple shadows
            params.shadowTintStrength = 0.3f * (1.0f - t);
            params.highlightTint = Vec3(1.0f, 0.85f, 0.6f);  // Golden highlights
            params.highlightTintStrength = 0.4f * (1.0f - t);
            params.saturation = 1.1f;
            params.temperature = 0.2f * (1.0f - t);
        }
        else if (dayTime > 0.65f && dayTime < 0.8f)
        {
            // Dusk - golden hour, orange/red tones
            f32 t = (dayTime - 0.65f) / 0.15f;
            params.shadowTint = Vec3(0.3f, 0.1f, 0.2f);  // Magenta shadows
            params.shadowTintStrength = 0.3f * t;
            params.highlightTint = Vec3(1.0f, 0.6f, 0.3f);  // Orange highlights
            params.highlightTintStrength = 0.5f * t;
            params.saturation = 1.15f;
            params.temperature = 0.3f * t;
        }
        else if (dayTime > 0.85f || dayTime < 0.15f)
        {
            // Night - cool, desaturated
            params.shadowTint = Vec3(0.1f, 0.1f, 0.2f);  // Deep blue shadows
            params.shadowTintStrength = 0.4f;
            params.highlightTint = Vec3(0.7f, 0.75f, 0.9f);  // Cool highlights
            params.highlightTintStrength = 0.2f;
            params.saturation = 0.7f;
            params.contrast = 0.9f;
            params.exposure = 0.8f;
            params.temperature = -0.15f;
        }

        return params;
    }

    // ========================================================================
    // Moon Intensity (for lighting)
    // ========================================================================

    f32 GetMoonIntensity() const
    {
        if (!IsNight()) return 0.0f;
        return GetMoonVisibility() * 0.15f;  // Max 15% of sun intensity
    }

private:
    f32 m_dayCount = 0.0f;
    f32 m_moonPhaseDay = 0.0f;  // Day within lunar cycle


    static Vec3 LerpVec3(const Vec3& a, const Vec3& b, f32 t)
    {
        return Vec3(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        );
    }

    static f32 LerpF32(f32 a, f32 b, f32 t)
    {
        return a + (b - a) * t;
    }
};
