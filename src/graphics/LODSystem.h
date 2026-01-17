#pragma once

// LODSystem.h - Centralized Level-of-Detail management for all renderable objects
// Handles distance-based LOD, fade transitions, and quality scaling

#include <glm/glm.hpp>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace LOD {

// ============================================================================
// LOD Levels for different object types
// ============================================================================

enum class TreeLOD : uint8_t {
    FULL_MESH = 0,      // < 100m - Full procedural mesh with all branches/leaves
    SIMPLIFIED = 1,     // < 300m - Reduced branch count, merged leaves
    BILLBOARD = 2,      // < 800m - Camera-facing quad with tree texture
    IMPOSTOR = 3,       // < 1500m - Small billboard, no leaf detail
    POINT = 4,          // < 2500m - Colored point sprite
    CULLED = 5          // Beyond render distance
};

enum class GrassLOD : uint8_t {
    INDIVIDUAL = 0,     // < 50m  - Individual grass blades with full animation
    CLUSTERED = 1,      // < 150m - Clustered patches, fewer draw calls
    TEXTURED = 2,       // < 400m - Ground texture with grass pattern
    CULLED = 3          // Beyond render distance
};

enum class CreatureLOD : uint8_t {
    FULL_MESH = 0,      // < 30m  - Full procedural mesh with animations
    MEDIUM = 1,         // < 80m  - Simplified mesh (50% vertices)
    LOW = 2,            // < 150m - Low-poly silhouette
    BILLBOARD = 3,      // < 300m - Camera-facing sprite
    POINT = 4,          // < 500m - Colored point
    CULLED = 5          // Beyond render distance
};

// ============================================================================
// LOD Configuration - Adjustable distances and quality settings
// ============================================================================

struct LODConfig {
    // Tree LOD distances (much longer than before to fix pop-in)
    float treeFull = 100.0f;
    float treeSimplified = 300.0f;
    float treeBillboard = 800.0f;
    float treeImpostor = 1500.0f;
    float treePoint = 2500.0f;
    float treeMaxDistance = 3500.0f;

    // Grass LOD distances
    float grassIndividual = 50.0f;
    float grassClustered = 150.0f;
    float grassTextured = 400.0f;
    float grassMaxDistance = 600.0f;

    // Creature LOD distances
    float creatureFull = 30.0f;
    float creatureMedium = 80.0f;
    float creatureLow = 150.0f;
    float creatureBillboard = 300.0f;
    float creaturePoint = 500.0f;
    float creatureMaxDistance = 800.0f;

    // Fade transition distances (smooth LOD switching)
    float treeFadeRange = 50.0f;     // Distance over which tree LOD fades
    float grassFadeRange = 20.0f;
    float creatureFadeRange = 15.0f;

    // Quality multiplier (0.5 = half distance, 2.0 = double)
    float qualityScale = 1.0f;

    // Fog settings for hiding LOD transitions
    float fogStart = 400.0f;
    float fogEnd = 2000.0f;
    float fogDensity = 0.0008f;
    glm::vec3 fogColor = glm::vec3(0.7f, 0.8f, 0.9f);

    // Screen-space LOD thresholds (pixels)
    float minPixelsForFull = 100.0f;
    float minPixelsForMedium = 40.0f;
    float minPixelsForLow = 15.0f;
    float minPixelsForBillboard = 5.0f;
    float minPixelsForPoint = 1.0f;

    // Apply quality scale to all distances
    void applyQualityScale(float scale) {
        qualityScale = scale;
    }

    float scaled(float distance) const {
        return distance * qualityScale;
    }
};

// Global LOD configuration (can be modified at runtime)
inline LODConfig& getConfig() {
    static LODConfig config;
    return config;
}

// ============================================================================
// LOD Calculation Functions
// ============================================================================

// Calculate tree LOD based on distance
inline TreeLOD calculateTreeLOD(float distance, const LODConfig& config = getConfig()) {
    float d = distance;
    float q = config.qualityScale;

    if (d < config.treeFull * q) return TreeLOD::FULL_MESH;
    if (d < config.treeSimplified * q) return TreeLOD::SIMPLIFIED;
    if (d < config.treeBillboard * q) return TreeLOD::BILLBOARD;
    if (d < config.treeImpostor * q) return TreeLOD::IMPOSTOR;
    if (d < config.treePoint * q) return TreeLOD::POINT;
    if (d < config.treeMaxDistance * q) return TreeLOD::POINT; // Show points up to max
    return TreeLOD::CULLED;
}

// Calculate grass LOD based on distance
inline GrassLOD calculateGrassLOD(float distance, const LODConfig& config = getConfig()) {
    float d = distance;
    float q = config.qualityScale;

    if (d < config.grassIndividual * q) return GrassLOD::INDIVIDUAL;
    if (d < config.grassClustered * q) return GrassLOD::CLUSTERED;
    if (d < config.grassTextured * q) return GrassLOD::TEXTURED;
    return GrassLOD::CULLED;
}

// Calculate creature LOD based on distance
inline CreatureLOD calculateCreatureLOD(float distance, const LODConfig& config = getConfig()) {
    float d = distance;
    float q = config.qualityScale;

    if (d < config.creatureFull * q) return CreatureLOD::FULL_MESH;
    if (d < config.creatureMedium * q) return CreatureLOD::MEDIUM;
    if (d < config.creatureLow * q) return CreatureLOD::LOW;
    if (d < config.creatureBillboard * q) return CreatureLOD::BILLBOARD;
    if (d < config.creaturePoint * q) return CreatureLOD::POINT;
    return CreatureLOD::CULLED;
}

// ============================================================================
// Fade/Transition Calculations
// ============================================================================

// Calculate fade factor for smooth LOD transitions (0 = fully faded out, 1 = fully visible)
inline float calculateFadeFactor(float distance, float lodStart, float fadeRange) {
    if (distance < lodStart) return 1.0f;
    if (distance > lodStart + fadeRange) return 0.0f;
    return 1.0f - (distance - lodStart) / fadeRange;
}

// Calculate tree fade factor at current LOD boundary
inline float calculateTreeFade(float distance, TreeLOD currentLOD, const LODConfig& config = getConfig()) {
    float q = config.qualityScale;
    float fade = config.treeFadeRange;

    switch (currentLOD) {
        case TreeLOD::FULL_MESH:
            return calculateFadeFactor(distance, config.treeFull * q, fade);
        case TreeLOD::SIMPLIFIED:
            return calculateFadeFactor(distance, config.treeSimplified * q, fade);
        case TreeLOD::BILLBOARD:
            return calculateFadeFactor(distance, config.treeBillboard * q, fade);
        case TreeLOD::IMPOSTOR:
            return calculateFadeFactor(distance, config.treeImpostor * q, fade);
        case TreeLOD::POINT:
            return calculateFadeFactor(distance, config.treePoint * q, fade * 2.0f);
        default:
            return 0.0f;
    }
}

// Calculate grass fade factor
inline float calculateGrassFade(float distance, GrassLOD currentLOD, const LODConfig& config = getConfig()) {
    float q = config.qualityScale;
    float fade = config.grassFadeRange;

    switch (currentLOD) {
        case GrassLOD::INDIVIDUAL:
            return calculateFadeFactor(distance, config.grassIndividual * q, fade);
        case GrassLOD::CLUSTERED:
            return calculateFadeFactor(distance, config.grassClustered * q, fade);
        case GrassLOD::TEXTURED:
            return calculateFadeFactor(distance, config.grassTextured * q, fade * 2.0f);
        default:
            return 0.0f;
    }
}

// Calculate creature fade factor
inline float calculateCreatureFade(float distance, CreatureLOD currentLOD, const LODConfig& config = getConfig()) {
    float q = config.qualityScale;
    float fade = config.creatureFadeRange;

    switch (currentLOD) {
        case CreatureLOD::FULL_MESH:
            return calculateFadeFactor(distance, config.creatureFull * q, fade);
        case CreatureLOD::MEDIUM:
            return calculateFadeFactor(distance, config.creatureMedium * q, fade);
        case CreatureLOD::LOW:
            return calculateFadeFactor(distance, config.creatureLow * q, fade);
        case CreatureLOD::BILLBOARD:
            return calculateFadeFactor(distance, config.creatureBillboard * q, fade);
        case CreatureLOD::POINT:
            return calculateFadeFactor(distance, config.creaturePoint * q, fade * 2.0f);
        default:
            return 0.0f;
    }
}

// ============================================================================
// Distance Fog Calculations
// ============================================================================

// Calculate fog factor (0 = no fog, 1 = full fog)
inline float calculateFogFactor(float distance, const LODConfig& config = getConfig()) {
    // Exponential fog
    float fogAmount = 1.0f - std::exp(-config.fogDensity * distance);

    // Linear blend at boundaries
    if (distance < config.fogStart) {
        fogAmount *= distance / config.fogStart;
    }

    return std::clamp(fogAmount, 0.0f, 1.0f);
}

// Apply fog to color
inline glm::vec3 applyFog(const glm::vec3& color, float distance, const LODConfig& config = getConfig()) {
    float fogFactor = calculateFogFactor(distance, config);
    return glm::mix(color, config.fogColor, fogFactor);
}

// ============================================================================
// Screen-Space LOD (based on projected size)
// ============================================================================

// Calculate projected screen size in pixels
inline float calculateScreenSize(float worldSize, float distance, float fovY, float screenHeight) {
    if (distance < 0.001f) return screenHeight;
    float tanHalfFov = std::tan(fovY * 0.5f);
    return (worldSize / distance) * (screenHeight * 0.5f / tanHalfFov);
}

// Get LOD based on screen size (more accurate than distance alone)
inline CreatureLOD calculateCreatureLODScreenSpace(float screenPixels, const LODConfig& config = getConfig()) {
    if (screenPixels >= config.minPixelsForFull) return CreatureLOD::FULL_MESH;
    if (screenPixels >= config.minPixelsForMedium) return CreatureLOD::MEDIUM;
    if (screenPixels >= config.minPixelsForLow) return CreatureLOD::LOW;
    if (screenPixels >= config.minPixelsForBillboard) return CreatureLOD::BILLBOARD;
    if (screenPixels >= config.minPixelsForPoint) return CreatureLOD::POINT;
    return CreatureLOD::CULLED;
}

// ============================================================================
// LOD Statistics for debugging
// ============================================================================

struct LODStats {
    // Tree counts by LOD
    uint32_t treesFullMesh = 0;
    uint32_t treesSimplified = 0;
    uint32_t treesBillboard = 0;
    uint32_t treesImpostor = 0;
    uint32_t treesPoint = 0;
    uint32_t treesCulled = 0;

    // Grass counts by LOD
    uint32_t grassIndividual = 0;
    uint32_t grassClustered = 0;
    uint32_t grassTextured = 0;
    uint32_t grassCulled = 0;

    // Creature counts by LOD
    uint32_t creaturesFullMesh = 0;
    uint32_t creaturesMedium = 0;
    uint32_t creaturesLow = 0;
    uint32_t creaturesBillboard = 0;
    uint32_t creaturesPoint = 0;
    uint32_t creaturesCulled = 0;

    // Draw call counts
    uint32_t treeDrawCalls = 0;
    uint32_t grassDrawCalls = 0;
    uint32_t creatureDrawCalls = 0;

    void reset() {
        treesFullMesh = treesSimplified = treesBillboard = treesImpostor = treesPoint = treesCulled = 0;
        grassIndividual = grassClustered = grassTextured = grassCulled = 0;
        creaturesFullMesh = creaturesMedium = creaturesLow = creaturesBillboard = creaturesPoint = creaturesCulled = 0;
        treeDrawCalls = grassDrawCalls = creatureDrawCalls = 0;
    }

    uint32_t totalTreesVisible() const {
        return treesFullMesh + treesSimplified + treesBillboard + treesImpostor + treesPoint;
    }

    uint32_t totalGrassVisible() const {
        return grassIndividual + grassClustered + grassTextured;
    }

    uint32_t totalCreaturesVisible() const {
        return creaturesFullMesh + creaturesMedium + creaturesLow + creaturesBillboard + creaturesPoint;
    }
};

// Global stats (updated each frame)
inline LODStats& getStats() {
    static LODStats stats;
    return stats;
}

} // namespace LOD
