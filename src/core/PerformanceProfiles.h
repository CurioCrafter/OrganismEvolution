#pragma once

// PerformanceProfiles - Hardware-specific performance configurations
// Provides optimized presets for different GPU tiers targeting 60 FPS

#include "QualityScaler.h"
#include "PerformanceManager.h"
#include "CreatureUpdateScheduler.h"
#include "../graphics/RenderingOptimizer.h"
#include "../graphics/LODSystem.h"

namespace Forge {

// ============================================================================
// Hardware Performance Profiles
// ============================================================================

enum class HardwareProfile {
    RTX_3080,       // NVIDIA RTX 3080 - High-end target
    RTX_3070,       // NVIDIA RTX 3070 - Upper mid-range
    RTX_3060,       // NVIDIA RTX 3060 - Mid-range
    RTX_2060,       // NVIDIA RTX 2060 - Lower mid-range
    GTX_1660,       // NVIDIA GTX 1660 - Budget
    INTEGRATED,     // Integrated graphics - Minimal settings
    CUSTOM          // User-defined
};

// ============================================================================
// Complete Performance Configuration
// ============================================================================

struct PerformanceProfile {
    const char* name = "Custom";
    HardwareProfile hardware = HardwareProfile::CUSTOM;

    // Target performance
    float targetFPS = 60.0f;
    float minAcceptableFPS = 55.0f;
    float maxAcceptableFPS = 58.0f;

    // Quality settings
    QualityPreset defaultPreset = QualityPreset::MEDIUM;
    bool enableAdaptiveQuality = true;
    float qualityScaleMin = 0.5f;
    float qualityScaleMax = 1.2f;

    // Population limits
    int maxCreatures = 5000;
    int maxVisibleCreatures = 2000;
    int maxParticles = 30000;

    // LOD configuration
    LODThresholds lodThresholds{};

    // Update scheduler configuration
    UpdateSchedulerConfig schedulerConfig{};

    // Rendering configuration
    RenderingConfig renderingConfig{};

    // LOD system configuration
    LOD::LODConfig lodSystemConfig{};

    // Graphics quality
    int shadowResolution = 2048;
    int shadowCascades = 3;
    float shadowDistance = 150.0f;
    bool enableSSAO = true;
    bool enableSSR = false;
    bool enableVolumetrics = false;
    bool enableBloom = true;

    // Vegetation density
    float vegetationDensity = 0.75f;
    float grassDensity = 0.5f;

    // Apply profile to quality settings
    QualitySettings toQualitySettings() const {
        QualitySettings settings;
        settings.maxCreatures = maxCreatures;
        settings.maxVisibleCreatures = maxVisibleCreatures;
        settings.maxParticles = maxParticles;
        settings.shadowResolution = shadowResolution;
        settings.shadowCascades = shadowCascades;
        settings.shadowDistance = shadowDistance;
        settings.enableShadows = (shadowResolution > 0);
        settings.enableSSAO = enableSSAO;
        settings.enableSSR = enableSSR;
        settings.enableVolumetrics = enableVolumetrics;
        settings.enableBloom = enableBloom;
        settings.vegetationDensity = vegetationDensity;
        settings.grassDensity = grassDensity;
        return settings;
    }
};

// ============================================================================
// RTX 3080 Profile - Optimized for high population at 60 FPS
// ============================================================================

inline PerformanceProfile getRTX3080Profile() {
    PerformanceProfile profile;
    profile.name = "RTX 3080";
    profile.hardware = HardwareProfile::RTX_3080;

    // Performance targets
    profile.targetFPS = 60.0f;
    profile.minAcceptableFPS = 55.0f;
    profile.maxAcceptableFPS = 58.0f;

    // Quality settings
    profile.defaultPreset = QualityPreset::HIGH;
    profile.enableAdaptiveQuality = true;
    profile.qualityScaleMin = 0.7f;   // Don't go below 70% quality
    profile.qualityScaleMax = 1.3f;   // Allow up to 130% for extra quality

    // Population (RTX 3080 can handle high counts)
    profile.maxCreatures = 10000;
    profile.maxVisibleCreatures = 5000;
    profile.maxParticles = 50000;

    // LOD thresholds (balanced for large populations)
    profile.lodThresholds.fullToMedium = 50.0f;
    profile.lodThresholds.mediumToLow = 100.0f;
    profile.lodThresholds.lowToBillboard = 200.0f;
    profile.lodThresholds.billboardToCulled = 400.0f;

    // Update scheduler configuration
    profile.schedulerConfig.criticalDistance = 30.0f;
    profile.schedulerConfig.highDistance = 80.0f;
    profile.schedulerConfig.mediumDistance = 150.0f;
    profile.schedulerConfig.lowDistance = 300.0f;
    profile.schedulerConfig.minimalDistance = 500.0f;
    profile.schedulerConfig.criticalInterval = 1;
    profile.schedulerConfig.highInterval = 1;
    profile.schedulerConfig.mediumInterval = 2;
    profile.schedulerConfig.lowInterval = 4;
    profile.schedulerConfig.minimalInterval = 8;
    profile.schedulerConfig.dormantInterval = 16;
    profile.schedulerConfig.adaptiveScheduling = true;

    // Rendering configuration
    profile.renderingConfig.highLODDistance = 40.0f;
    profile.renderingConfig.mediumLODDistance = 100.0f;
    profile.renderingConfig.lowLODDistance = 180.0f;
    profile.renderingConfig.billboardDistance = 350.0f;
    profile.renderingConfig.pointDistance = 600.0f;
    profile.renderingConfig.maxInstancesPerBatch = 4096;
    profile.renderingConfig.enableFrustumCulling = true;
    profile.renderingConfig.enableDistanceCulling = true;
    profile.renderingConfig.enableOcclusionCulling = false;  // Too expensive
    profile.renderingConfig.enableScreenSpaceLOD = true;
    profile.renderingConfig.enableMegaBatching = true;

    // LOD system configuration (synced)
    profile.lodSystemConfig.creatureFull = 40.0f;
    profile.lodSystemConfig.creatureMedium = 100.0f;
    profile.lodSystemConfig.creatureLow = 180.0f;
    profile.lodSystemConfig.creatureBillboard = 350.0f;
    profile.lodSystemConfig.creaturePoint = 600.0f;
    profile.lodSystemConfig.creatureMaxDistance = 1000.0f;
    profile.lodSystemConfig.creatureFadeRange = 20.0f;
    profile.lodSystemConfig.qualityScale = 1.0f;

    // Graphics quality (high settings for RTX 3080)
    profile.shadowResolution = 2048;
    profile.shadowCascades = 4;
    profile.shadowDistance = 200.0f;
    profile.enableSSAO = true;
    profile.enableSSR = false;  // Expensive, save for ultra
    profile.enableVolumetrics = false;  // Very expensive
    profile.enableBloom = true;

    // Vegetation
    profile.vegetationDensity = 1.0f;
    profile.grassDensity = 0.75f;

    return profile;
}

// ============================================================================
// RTX 3070 Profile - Slightly reduced from 3080
// ============================================================================

inline PerformanceProfile getRTX3070Profile() {
    PerformanceProfile profile = getRTX3080Profile();
    profile.name = "RTX 3070";
    profile.hardware = HardwareProfile::RTX_3070;

    // Slightly lower population
    profile.maxCreatures = 8000;
    profile.maxVisibleCreatures = 4000;
    profile.maxParticles = 40000;

    // Reduce LOD distances slightly
    profile.lodThresholds.fullToMedium = 45.0f;
    profile.lodThresholds.mediumToLow = 90.0f;

    profile.schedulerConfig.criticalDistance = 25.0f;
    profile.schedulerConfig.highDistance = 70.0f;
    profile.schedulerConfig.mediumDistance = 130.0f;

    profile.renderingConfig.highLODDistance = 35.0f;
    profile.renderingConfig.mediumLODDistance = 90.0f;
    profile.renderingConfig.lowLODDistance = 160.0f;

    // Sync LOD system
    profile.lodSystemConfig.creatureFull = 35.0f;
    profile.lodSystemConfig.creatureMedium = 90.0f;
    profile.lodSystemConfig.creatureLow = 160.0f;

    // Slightly reduce shadow quality
    profile.shadowDistance = 180.0f;
    profile.grassDensity = 0.65f;

    return profile;
}

// ============================================================================
// RTX 3060 Profile - Mid-range target
// ============================================================================

inline PerformanceProfile getRTX3060Profile() {
    PerformanceProfile profile = getRTX3080Profile();
    profile.name = "RTX 3060";
    profile.hardware = HardwareProfile::RTX_3060;
    profile.defaultPreset = QualityPreset::MEDIUM;

    // Medium population
    profile.maxCreatures = 6000;
    profile.maxVisibleCreatures = 3000;
    profile.maxParticles = 30000;

    // Moderate LOD distances
    profile.lodThresholds.fullToMedium = 40.0f;
    profile.lodThresholds.mediumToLow = 80.0f;
    profile.lodThresholds.lowToBillboard = 150.0f;
    profile.lodThresholds.billboardToCulled = 300.0f;

    profile.schedulerConfig.criticalDistance = 20.0f;
    profile.schedulerConfig.highDistance = 60.0f;
    profile.schedulerConfig.mediumDistance = 120.0f;
    profile.schedulerConfig.lowDistance = 250.0f;

    profile.renderingConfig.highLODDistance = 30.0f;
    profile.renderingConfig.mediumLODDistance = 80.0f;
    profile.renderingConfig.lowLODDistance = 140.0f;
    profile.renderingConfig.billboardDistance = 280.0f;

    profile.lodSystemConfig.creatureFull = 30.0f;
    profile.lodSystemConfig.creatureMedium = 80.0f;
    profile.lodSystemConfig.creatureLow = 140.0f;
    profile.lodSystemConfig.creatureBillboard = 280.0f;

    // Medium graphics
    profile.shadowResolution = 2048;
    profile.shadowCascades = 3;
    profile.shadowDistance = 150.0f;
    profile.enableSSAO = true;
    profile.enableSSR = false;
    profile.vegetationDensity = 0.75f;
    profile.grassDensity = 0.5f;

    return profile;
}

// ============================================================================
// RTX 2060 Profile - Lower mid-range
// ============================================================================

inline PerformanceProfile getRTX2060Profile() {
    PerformanceProfile profile = getRTX3060Profile();
    profile.name = "RTX 2060";
    profile.hardware = HardwareProfile::RTX_2060;
    profile.defaultPreset = QualityPreset::MEDIUM;

    profile.maxCreatures = 4000;
    profile.maxVisibleCreatures = 2000;
    profile.maxParticles = 20000;

    profile.lodThresholds.fullToMedium = 35.0f;
    profile.lodThresholds.mediumToLow = 70.0f;

    profile.schedulerConfig.criticalDistance = 18.0f;
    profile.schedulerConfig.highDistance = 50.0f;
    profile.schedulerConfig.mediumDistance = 100.0f;

    profile.renderingConfig.highLODDistance = 25.0f;
    profile.renderingConfig.mediumLODDistance = 70.0f;
    profile.renderingConfig.lowLODDistance = 120.0f;

    profile.lodSystemConfig.creatureFull = 25.0f;
    profile.lodSystemConfig.creatureMedium = 70.0f;
    profile.lodSystemConfig.creatureLow = 120.0f;

    profile.shadowResolution = 1024;
    profile.shadowCascades = 2;
    profile.shadowDistance = 120.0f;
    profile.vegetationDensity = 0.6f;
    profile.grassDensity = 0.35f;

    return profile;
}

// ============================================================================
// GTX 1660 Profile - Budget GPU
// ============================================================================

inline PerformanceProfile getGTX1660Profile() {
    PerformanceProfile profile = getRTX2060Profile();
    profile.name = "GTX 1660";
    profile.hardware = HardwareProfile::GTX_1660;
    profile.defaultPreset = QualityPreset::LOW;

    profile.maxCreatures = 3000;
    profile.maxVisibleCreatures = 1500;
    profile.maxParticles = 15000;

    profile.lodThresholds.fullToMedium = 30.0f;
    profile.lodThresholds.mediumToLow = 60.0f;
    profile.lodThresholds.lowToBillboard = 120.0f;
    profile.lodThresholds.billboardToCulled = 250.0f;

    profile.schedulerConfig.criticalDistance = 15.0f;
    profile.schedulerConfig.highDistance = 40.0f;
    profile.schedulerConfig.mediumDistance = 80.0f;

    profile.renderingConfig.highLODDistance = 20.0f;
    profile.renderingConfig.mediumLODDistance = 60.0f;
    profile.renderingConfig.lowLODDistance = 100.0f;

    profile.lodSystemConfig.creatureFull = 20.0f;
    profile.lodSystemConfig.creatureMedium = 60.0f;
    profile.lodSystemConfig.creatureLow = 100.0f;

    profile.shadowResolution = 1024;
    profile.shadowCascades = 2;
    profile.shadowDistance = 100.0f;
    profile.enableSSAO = false;
    profile.vegetationDensity = 0.5f;
    profile.grassDensity = 0.25f;

    return profile;
}

// ============================================================================
// Integrated Graphics Profile - Minimal settings
// ============================================================================

inline PerformanceProfile getIntegratedProfile() {
    PerformanceProfile profile;
    profile.name = "Integrated Graphics";
    profile.hardware = HardwareProfile::INTEGRATED;
    profile.defaultPreset = QualityPreset::ULTRA_LOW;

    profile.targetFPS = 30.0f;  // Lower target for integrated
    profile.minAcceptableFPS = 25.0f;

    profile.maxCreatures = 1000;
    profile.maxVisibleCreatures = 500;
    profile.maxParticles = 5000;

    profile.lodThresholds.fullToMedium = 20.0f;
    profile.lodThresholds.mediumToLow = 40.0f;
    profile.lodThresholds.lowToBillboard = 80.0f;
    profile.lodThresholds.billboardToCulled = 150.0f;

    profile.schedulerConfig.criticalDistance = 10.0f;
    profile.schedulerConfig.highDistance = 30.0f;
    profile.schedulerConfig.mediumDistance = 60.0f;
    profile.schedulerConfig.lowDistance = 120.0f;

    profile.renderingConfig.highLODDistance = 15.0f;
    profile.renderingConfig.mediumLODDistance = 40.0f;
    profile.renderingConfig.lowLODDistance = 70.0f;
    profile.renderingConfig.billboardDistance = 120.0f;
    profile.renderingConfig.maxInstancesPerBatch = 2048;

    profile.lodSystemConfig.creatureFull = 15.0f;
    profile.lodSystemConfig.creatureMedium = 40.0f;
    profile.lodSystemConfig.creatureLow = 70.0f;
    profile.lodSystemConfig.creatureBillboard = 120.0f;

    profile.shadowResolution = 512;
    profile.shadowCascades = 1;
    profile.shadowDistance = 50.0f;
    profile.enableSSAO = false;
    profile.enableBloom = false;
    profile.vegetationDensity = 0.25f;
    profile.grassDensity = 0.0f;

    return profile;
}

// ============================================================================
// Profile Selection
// ============================================================================

inline PerformanceProfile getProfileForHardware(HardwareProfile hardware) {
    switch (hardware) {
        case HardwareProfile::RTX_3080: return getRTX3080Profile();
        case HardwareProfile::RTX_3070: return getRTX3070Profile();
        case HardwareProfile::RTX_3060: return getRTX3060Profile();
        case HardwareProfile::RTX_2060: return getRTX2060Profile();
        case HardwareProfile::GTX_1660: return getGTX1660Profile();
        case HardwareProfile::INTEGRATED: return getIntegratedProfile();
        default: return getRTX3060Profile();  // Default to mid-range
    }
}

inline const char* getHardwareProfileName(HardwareProfile hardware) {
    return getProfileForHardware(hardware).name;
}

} // namespace Forge
