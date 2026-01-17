#pragma once

// QualityScaler - Dynamic quality scaling based on FPS to maintain 60 FPS target
// Automatically adjusts rendering quality, creature limits, and effects

#include <vector>
#include <array>
#include <algorithm>
#include <numeric>

namespace Forge {

// ============================================================================
// Quality Presets
// ============================================================================

enum class QualityPreset {
    ULTRA_LOW,    // Emergency mode - minimal everything
    LOW,          // For struggling hardware
    MEDIUM,       // Balanced
    HIGH,         // Good performance with quality
    ULTRA,        // Maximum quality
    CUSTOM,       // User-defined
    COUNT
};

// ============================================================================
// Quality Settings
// ============================================================================

struct QualitySettings {
    // Creature limits
    int maxCreatures = 5000;
    int maxVisibleCreatures = 2000;
    int maxParticlesPerCreature = 20;

    // LOD distances (multiplier applied to base distances)
    float lodBias = 1.0f;
    float updateDistanceBias = 1.0f;

    // Rendering
    int shadowResolution = 2048;
    int shadowCascades = 4;
    float shadowDistance = 200.0f;
    bool enableShadows = true;
    bool enableSSAO = true;
    bool enableSSR = false;          // Screen-space reflections
    bool enableVolumetrics = false;   // Volumetric lighting
    bool enableMotionBlur = false;
    bool enableBloom = true;
    bool enableDOF = false;           // Depth of field

    // Particles
    int maxParticles = 50000;
    int maxParticleEmitters = 100;
    float particleDensity = 1.0f;

    // Vegetation
    float vegetationDensity = 1.0f;
    float grassDensity = 1.0f;
    float vegetationDrawDistance = 300.0f;
    bool enableGrassAnimation = true;

    // Water
    int waterReflectionResolution = 512;
    int waterRefractionResolution = 512;
    bool enableWaterReflections = true;
    bool enableWaterRefractions = true;
    bool enableWaterCaustics = false;

    // Audio
    int maxAudioVoices = 32;
    float audioRange = 100.0f;

    // AI/Behavior
    int maxBehaviorUpdatesPerFrame = 1000;
    bool enableAdvancedAI = true;
    bool enableFlocking = true;

    // Physics
    float physicsTimestep = 1.0f / 60.0f;
    int maxPhysicsSubsteps = 4;
};

// ============================================================================
// Quality Scaler Configuration
// ============================================================================

struct QualityScalerConfig {
    // Target performance
    float targetFPS = 60.0f;
    float minAcceptableFPS = 55.0f;     // Below this, reduce quality
    float maxAcceptableFPS = 58.0f;     // Above this, can increase quality

    // Scaling behavior
    float scaleDownRate = 0.02f;        // How fast to reduce quality
    float scaleUpRate = 0.01f;          // How fast to increase quality (slower)
    int historyFrames = 60;             // Frames to average for decisions
    int stabilityFrames = 30;           // Frames to wait before changing quality

    // Limits
    QualityPreset minPreset = QualityPreset::ULTRA_LOW;
    QualityPreset maxPreset = QualityPreset::ULTRA;

    // Auto-adjustment
    bool autoAdjust = true;
    bool smoothTransitions = true;
};

// ============================================================================
// Quality Scaler Statistics
// ============================================================================

struct QualityScalerStats {
    // FPS tracking
    float currentFPS = 60.0f;
    float averageFPS = 60.0f;
    float minFPS = 60.0f;
    float maxFPS = 60.0f;
    float fps1PercentLow = 60.0f;      // 1% lows for stutter detection

    // Quality state
    QualityPreset currentPreset = QualityPreset::HIGH;
    float qualityLevel = 1.0f;          // 0.0 to 1.0 within current preset
    int presetChanges = 0;
    int framesSinceChange = 0;

    // Performance breakdown (for debugging)
    float gpuBoundFrames = 0.0f;        // % of frames GPU-bound
    float cpuBoundFrames = 0.0f;        // % of frames CPU-bound
};

// ============================================================================
// Quality Scaler
// ============================================================================

class QualityScaler {
public:
    QualityScaler();
    ~QualityScaler() = default;

    // Configuration
    void setConfig(const QualityScalerConfig& config) { m_config = config; }
    const QualityScalerConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Update with current frame time (call every frame)
    void update(float frameTimeMs);

    // Get current quality settings
    const QualitySettings& getCurrentSettings() const { return m_currentSettings; }

    // Get current quality preset
    QualityPreset getCurrentPreset() const { return m_stats.currentPreset; }

    // ========================================================================
    // Manual Control
    // ========================================================================

    // Set quality preset manually (disables auto-adjust until next setConfig)
    void setPreset(QualityPreset preset);

    // Set quality level within current preset (0.0 = low end, 1.0 = high end)
    void setQualityLevel(float level);

    // Force refresh of settings
    void refreshSettings();

    // Enable/disable auto-adjustment
    void setAutoAdjust(bool enabled) { m_config.autoAdjust = enabled; }
    bool isAutoAdjust() const { return m_config.autoAdjust; }

    // ========================================================================
    // Statistics
    // ========================================================================

    const QualityScalerStats& getStats() const { return m_stats; }

    // Preset name for UI
    static const char* getPresetName(QualityPreset preset);

    // ========================================================================
    // Preset Definitions
    // ========================================================================

    static QualitySettings getPresetSettings(QualityPreset preset);

private:
    QualityScalerConfig m_config;
    QualitySettings m_currentSettings;
    QualityScalerStats m_stats;

    // FPS history
    std::vector<float> m_fpsHistory;
    size_t m_fpsHistoryIndex = 0;

    // Stability tracking
    int m_framesSinceChange = 0;
    bool m_stable = true;

    // Helper methods
    void updateFPSHistory(float fps);
    void calculateFPSStats();
    void autoAdjustQuality();
    void interpolateSettings(const QualitySettings& from, const QualitySettings& to, float t);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline QualityScaler::QualityScaler() {
    m_fpsHistory.resize(m_config.historyFrames, 60.0f);
    m_currentSettings = getPresetSettings(QualityPreset::HIGH);
    m_stats.currentPreset = QualityPreset::HIGH;
}

inline const char* QualityScaler::getPresetName(QualityPreset preset) {
    static const char* names[] = {
        "Ultra Low", "Low", "Medium", "High", "Ultra", "Custom"
    };
    return names[static_cast<size_t>(preset)];
}

inline void QualityScaler::update(float frameTimeMs) {
    float fps = (frameTimeMs > 0.001f) ? 1000.0f / frameTimeMs : 60.0f;

    updateFPSHistory(fps);
    calculateFPSStats();

    m_framesSinceChange++;

    if (m_config.autoAdjust && m_framesSinceChange >= m_config.stabilityFrames) {
        autoAdjustQuality();
    }
}

inline void QualityScaler::updateFPSHistory(float fps) {
    m_fpsHistory[m_fpsHistoryIndex] = fps;
    m_fpsHistoryIndex = (m_fpsHistoryIndex + 1) % m_fpsHistory.size();
}

inline void QualityScaler::calculateFPSStats() {
    // Calculate average
    float sum = std::accumulate(m_fpsHistory.begin(), m_fpsHistory.end(), 0.0f);
    m_stats.averageFPS = sum / m_fpsHistory.size();

    // Find min/max
    auto [minIt, maxIt] = std::minmax_element(m_fpsHistory.begin(), m_fpsHistory.end());
    m_stats.minFPS = *minIt;
    m_stats.maxFPS = *maxIt;

    // Calculate 1% lows (worst 1% of frames)
    std::vector<float> sorted = m_fpsHistory;
    std::sort(sorted.begin(), sorted.end());
    size_t onePercentCount = std::max(size_t(1), sorted.size() / 100);
    float lowSum = 0.0f;
    for (size_t i = 0; i < onePercentCount; ++i) {
        lowSum += sorted[i];
    }
    m_stats.fps1PercentLow = lowSum / onePercentCount;

    m_stats.currentFPS = m_fpsHistory[(m_fpsHistoryIndex + m_fpsHistory.size() - 1) % m_fpsHistory.size()];
    m_stats.framesSinceChange = m_framesSinceChange;
}

inline void QualityScaler::autoAdjustQuality() {
    float avgFPS = m_stats.averageFPS;
    float lowFPS = m_stats.fps1PercentLow;

    // Check if we need to reduce quality (prioritize 1% lows to avoid stuttering)
    if (lowFPS < m_config.minAcceptableFPS * 0.9f || avgFPS < m_config.minAcceptableFPS) {
        // Reduce quality
        int currentPresetInt = static_cast<int>(m_stats.currentPreset);
        int minPresetInt = static_cast<int>(m_config.minPreset);

        if (currentPresetInt > minPresetInt) {
            m_stats.qualityLevel -= m_config.scaleDownRate;

            if (m_stats.qualityLevel <= 0.0f) {
                // Move to lower preset
                setPreset(static_cast<QualityPreset>(currentPresetInt - 1));
                m_stats.qualityLevel = 1.0f;
            } else {
                refreshSettings();
            }
            m_framesSinceChange = 0;
            m_stats.presetChanges++;
        }
    }
    // Check if we can increase quality
    else if (avgFPS > m_config.maxAcceptableFPS && lowFPS > m_config.minAcceptableFPS) {
        // Increase quality (more conservatively)
        int currentPresetInt = static_cast<int>(m_stats.currentPreset);
        int maxPresetInt = static_cast<int>(m_config.maxPreset);

        if (currentPresetInt < maxPresetInt || m_stats.qualityLevel < 1.0f) {
            m_stats.qualityLevel += m_config.scaleUpRate;

            if (m_stats.qualityLevel >= 1.0f && currentPresetInt < maxPresetInt) {
                // Move to higher preset
                setPreset(static_cast<QualityPreset>(currentPresetInt + 1));
                m_stats.qualityLevel = 0.0f;
            } else {
                m_stats.qualityLevel = std::min(1.0f, m_stats.qualityLevel);
                refreshSettings();
            }
            m_framesSinceChange = 0;
        }
    }
}

inline void QualityScaler::setPreset(QualityPreset preset) {
    m_stats.currentPreset = preset;
    m_currentSettings = getPresetSettings(preset);
    m_framesSinceChange = 0;
}

inline void QualityScaler::setQualityLevel(float level) {
    m_stats.qualityLevel = std::clamp(level, 0.0f, 1.0f);
    refreshSettings();
}

inline void QualityScaler::refreshSettings() {
    // Interpolate between current preset and next lower preset
    int presetInt = static_cast<int>(m_stats.currentPreset);
    int lowerPreset = std::max(0, presetInt - 1);

    QualitySettings high = getPresetSettings(m_stats.currentPreset);
    QualitySettings low = getPresetSettings(static_cast<QualityPreset>(lowerPreset));

    interpolateSettings(low, high, m_stats.qualityLevel);
}

inline void QualityScaler::interpolateSettings(const QualitySettings& from,
                                                const QualitySettings& to, float t) {
    // Interpolate numerical values
    m_currentSettings.maxCreatures = static_cast<int>(
        from.maxCreatures + (to.maxCreatures - from.maxCreatures) * t);
    m_currentSettings.maxVisibleCreatures = static_cast<int>(
        from.maxVisibleCreatures + (to.maxVisibleCreatures - from.maxVisibleCreatures) * t);
    m_currentSettings.maxParticles = static_cast<int>(
        from.maxParticles + (to.maxParticles - from.maxParticles) * t);

    m_currentSettings.lodBias = from.lodBias + (to.lodBias - from.lodBias) * t;
    m_currentSettings.shadowDistance = from.shadowDistance + (to.shadowDistance - from.shadowDistance) * t;
    m_currentSettings.vegetationDensity = from.vegetationDensity + (to.vegetationDensity - from.vegetationDensity) * t;
    m_currentSettings.particleDensity = from.particleDensity + (to.particleDensity - from.particleDensity) * t;

    // Boolean features use threshold
    m_currentSettings.enableSSAO = t > 0.5f ? to.enableSSAO : from.enableSSAO;
    m_currentSettings.enableShadows = t > 0.5f ? to.enableShadows : from.enableShadows;
    m_currentSettings.enableBloom = t > 0.5f ? to.enableBloom : from.enableBloom;

    // Discrete values round
    m_currentSettings.shadowResolution = t > 0.5f ? to.shadowResolution : from.shadowResolution;
    m_currentSettings.shadowCascades = t > 0.5f ? to.shadowCascades : from.shadowCascades;
}

inline QualitySettings QualityScaler::getPresetSettings(QualityPreset preset) {
    QualitySettings settings;

    switch (preset) {
        case QualityPreset::ULTRA_LOW:
            settings.maxCreatures = 1000;
            settings.maxVisibleCreatures = 500;
            settings.maxParticles = 5000;
            settings.maxParticlesPerCreature = 5;
            settings.lodBias = 0.5f;
            settings.shadowResolution = 512;
            settings.shadowCascades = 1;
            settings.shadowDistance = 50.0f;
            settings.enableShadows = false;
            settings.enableSSAO = false;
            settings.enableSSR = false;
            settings.enableVolumetrics = false;
            settings.enableBloom = false;
            settings.vegetationDensity = 0.25f;
            settings.grassDensity = 0.0f;
            settings.enableGrassAnimation = false;
            settings.enableWaterReflections = false;
            settings.maxAudioVoices = 8;
            settings.maxBehaviorUpdatesPerFrame = 200;
            settings.enableAdvancedAI = false;
            settings.enableFlocking = false;
            break;

        case QualityPreset::LOW:
            settings.maxCreatures = 2000;
            settings.maxVisibleCreatures = 1000;
            settings.maxParticles = 15000;
            settings.maxParticlesPerCreature = 10;
            settings.lodBias = 0.7f;
            settings.shadowResolution = 1024;
            settings.shadowCascades = 2;
            settings.shadowDistance = 100.0f;
            settings.enableShadows = true;
            settings.enableSSAO = false;
            settings.enableBloom = false;
            settings.vegetationDensity = 0.5f;
            settings.grassDensity = 0.25f;
            settings.enableGrassAnimation = false;
            settings.enableWaterReflections = false;
            settings.maxAudioVoices = 16;
            settings.maxBehaviorUpdatesPerFrame = 400;
            settings.enableFlocking = true;
            break;

        case QualityPreset::MEDIUM:
            settings.maxCreatures = 5000;
            settings.maxVisibleCreatures = 2000;
            settings.maxParticles = 30000;
            settings.maxParticlesPerCreature = 20;
            settings.lodBias = 1.0f;
            settings.shadowResolution = 2048;
            settings.shadowCascades = 3;
            settings.shadowDistance = 150.0f;
            settings.enableShadows = true;
            settings.enableSSAO = true;
            settings.enableBloom = true;
            settings.vegetationDensity = 0.75f;
            settings.grassDensity = 0.5f;
            settings.enableGrassAnimation = true;
            settings.enableWaterReflections = true;
            settings.waterReflectionResolution = 256;
            settings.maxAudioVoices = 24;
            settings.maxBehaviorUpdatesPerFrame = 800;
            break;

        case QualityPreset::HIGH:
            settings.maxCreatures = 7500;
            settings.maxVisibleCreatures = 3500;
            settings.maxParticles = 50000;
            settings.maxParticlesPerCreature = 30;
            settings.lodBias = 1.2f;
            settings.shadowResolution = 2048;
            settings.shadowCascades = 4;
            settings.shadowDistance = 200.0f;
            settings.enableShadows = true;
            settings.enableSSAO = true;
            settings.enableBloom = true;
            settings.vegetationDensity = 1.0f;
            settings.grassDensity = 0.75f;
            settings.enableGrassAnimation = true;
            settings.enableWaterReflections = true;
            settings.enableWaterRefractions = true;
            settings.waterReflectionResolution = 512;
            settings.maxAudioVoices = 32;
            settings.maxBehaviorUpdatesPerFrame = 1000;
            break;

        case QualityPreset::ULTRA:
            settings.maxCreatures = 10000;
            settings.maxVisibleCreatures = 5000;
            settings.maxParticles = 100000;
            settings.maxParticlesPerCreature = 50;
            settings.lodBias = 1.5f;
            settings.shadowResolution = 4096;
            settings.shadowCascades = 4;
            settings.shadowDistance = 300.0f;
            settings.enableShadows = true;
            settings.enableSSAO = true;
            settings.enableSSR = true;
            settings.enableVolumetrics = true;
            settings.enableBloom = true;
            settings.enableDOF = true;
            settings.vegetationDensity = 1.0f;
            settings.grassDensity = 1.0f;
            settings.vegetationDrawDistance = 500.0f;
            settings.enableGrassAnimation = true;
            settings.enableWaterReflections = true;
            settings.enableWaterRefractions = true;
            settings.enableWaterCaustics = true;
            settings.waterReflectionResolution = 1024;
            settings.maxAudioVoices = 64;
            settings.maxBehaviorUpdatesPerFrame = 2000;
            settings.enableAdvancedAI = true;
            break;

        default:
            break;
    }

    return settings;
}

} // namespace Forge
