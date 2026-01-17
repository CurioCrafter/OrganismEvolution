#pragma once

// PerformanceDebugOverlay - Real-time performance metrics display
// Shows FPS, quality scale, LOD breakdown, and system status for debugging

#include "PerformanceIntegration.h"
#include "PerformanceManager.h"
#include "QualityScaler.h"
#include "CreatureUpdateScheduler.h"
#include "../graphics/RenderingOptimizer.h"
#include <string>
#include <sstream>
#include <iomanip>

namespace Forge {

// ============================================================================
// Debug Overlay Configuration
// ============================================================================

struct DebugOverlayConfig {
    bool showFPS = true;
    bool showQuality = true;
    bool showLOD = true;
    bool showUpdateTiers = true;
    bool showCulling = true;
    bool showBatching = true;
    bool showMemory = true;
    bool showDetailedStats = false;  // Extra verbose mode

    // Visual settings
    int decimals = 1;
    bool useColors = true;
};

// ============================================================================
// Performance Debug Overlay
// ============================================================================

class PerformanceDebugOverlay {
public:
    PerformanceDebugOverlay() = default;
    ~PerformanceDebugOverlay() = default;

    void setConfig(const DebugOverlayConfig& config) { m_config = config; }
    const DebugOverlayConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Text Generation
    // ========================================================================

    // Generate complete debug text from subsystems
    std::string generateDebugText(const PerformanceSubsystems& subsystems) const {
        std::stringstream ss;

        if (m_config.showFPS) {
            appendFPSSection(ss, subsystems);
        }

        if (m_config.showQuality) {
            appendQualitySection(ss, subsystems);
        }

        if (m_config.showLOD) {
            appendLODSection(ss, subsystems);
        }

        if (m_config.showUpdateTiers) {
            appendUpdateTierSection(ss, subsystems);
        }

        if (m_config.showCulling) {
            appendCullingSection(ss, subsystems);
        }

        if (m_config.showBatching) {
            appendBatchingSection(ss, subsystems);
        }

        if (m_config.showMemory) {
            appendMemorySection(ss, subsystems);
        }

        if (m_config.showDetailedStats) {
            appendDetailedStats(ss, subsystems);
        }

        return ss.str();
    }

    // Generate compact single-line summary
    std::string generateCompactSummary(const PerformanceSubsystems& subsystems) const {
        std::stringstream ss;

        const auto* scaler = subsystems.getQualityScaler();
        const auto* scheduler = subsystems.getScheduler();
        const auto* renderer = subsystems.getRenderingOptimizer();

        if (!scaler || !scheduler || !renderer) {
            return "Performance systems not initialized";
        }

        const auto& qStats = scaler->getStats();
        const auto& sStats = scheduler->getStats();
        const auto& rStats = renderer->getStats();

        ss << std::fixed << std::setprecision(m_config.decimals);
        ss << "FPS: " << qStats.currentFPS;
        ss << " | Avg: " << qStats.averageFPS;
        ss << " | 1% Low: " << qStats.fps1PercentLow;
        ss << " | Quality: " << QualityScaler::getPresetName(qStats.currentPreset);
        ss << " | Visible: " << rStats.visibleCreatures << "/" << rStats.totalCreatures;
        ss << " | Updates: " << sStats.totalUpdates;
        ss << " | Batches: " << rStats.totalBatches;

        return ss.str();
    }

private:
    DebugOverlayConfig m_config;

    void appendFPSSection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* scaler = subsystems.getQualityScaler();
        if (!scaler) return;

        const auto& stats = scaler->getStats();

        ss << "=== FRAME RATE ===\n";
        ss << std::fixed << std::setprecision(m_config.decimals);
        ss << "Current FPS: " << stats.currentFPS;

        // Color code based on FPS
        if (m_config.useColors) {
            if (stats.currentFPS >= 58.0f) ss << " [EXCELLENT]";
            else if (stats.currentFPS >= 50.0f) ss << " [GOOD]";
            else if (stats.currentFPS >= 40.0f) ss << " [OK]";
            else ss << " [POOR]";
        }
        ss << "\n";

        ss << "Average FPS: " << stats.averageFPS << "\n";
        ss << "Min FPS: " << stats.minFPS << "\n";
        ss << "Max FPS: " << stats.maxFPS << "\n";
        ss << "1% Low: " << stats.fps1PercentLow;

        if (m_config.useColors) {
            if (stats.fps1PercentLow >= 50.0f) ss << " [SMOOTH]";
            else if (stats.fps1PercentLow >= 40.0f) ss << " [MINOR STUTTERS]";
            else ss << " [STUTTERING]";
        }
        ss << "\n\n";
    }

    void appendQualitySection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* scaler = subsystems.getQualityScaler();
        if (!scaler) return;

        const auto& stats = scaler->getStats();
        const auto& settings = scaler->getCurrentSettings();

        ss << "=== QUALITY ===\n";
        ss << "Preset: " << QualityScaler::getPresetName(stats.currentPreset) << "\n";
        ss << "Quality Level: " << std::fixed << std::setprecision(2)
           << (stats.qualityLevel * 100.0f) << "%\n";
        ss << "Max Creatures: " << settings.maxCreatures << "\n";
        ss << "Max Visible: " << settings.maxVisibleCreatures << "\n";
        ss << "LOD Bias: " << settings.lodBias << "x\n";
        ss << "Shadows: " << (settings.enableShadows ? "ON" : "OFF");
        if (settings.enableShadows) {
            ss << " (" << settings.shadowResolution << "px)";
        }
        ss << "\n";
        ss << "SSAO: " << (settings.enableSSAO ? "ON" : "OFF") << "\n";
        ss << "Bloom: " << (settings.enableBloom ? "ON" : "OFF") << "\n";
        ss << "Preset Changes: " << stats.presetChanges << "\n";
        ss << "Frames Since Change: " << stats.framesSinceChange << "\n\n";
    }

    void appendLODSection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* renderer = subsystems.getRenderingOptimizer();
        if (!renderer) return;

        const auto& stats = renderer->getStats();

        ss << "=== LOD BREAKDOWN ===\n";
        ss << "Total Creatures: " << stats.totalCreatures << "\n";
        ss << "Visible: " << stats.visibleCreatures << "\n";
        ss << "  High LOD: " << stats.countByLOD[static_cast<size_t>(MeshLOD::HIGH)] << "\n";
        ss << "  Medium LOD: " << stats.countByLOD[static_cast<size_t>(MeshLOD::MEDIUM)] << "\n";
        ss << "  Low LOD: " << stats.countByLOD[static_cast<size_t>(MeshLOD::LOW)] << "\n";
        ss << "  Billboard: " << stats.countByLOD[static_cast<size_t>(MeshLOD::BILLBOARD)] << "\n";
        ss << "  Point: " << stats.countByLOD[static_cast<size_t>(MeshLOD::POINT)] << "\n";
        ss << "Culled: " << stats.countByLOD[static_cast<size_t>(MeshLOD::CULLED)] << "\n\n";
    }

    void appendUpdateTierSection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* scheduler = subsystems.getScheduler();
        if (!scheduler) return;

        const auto& stats = scheduler->getStats();

        ss << "=== UPDATE TIERS ===\n";
        ss << "Total Updates: " << stats.totalUpdates << " / " << stats.totalCreatures << "\n";
        ss << "Update Rate: " << std::fixed << std::setprecision(1)
           << (stats.updateRate * 100.0f) << "%\n";
        ss << "  Critical: " << stats.updatesThisFrame[static_cast<size_t>(UpdateTier::CRITICAL)]
           << " (" << stats.countByTier[static_cast<size_t>(UpdateTier::CRITICAL)] << " total)\n";
        ss << "  High: " << stats.updatesThisFrame[static_cast<size_t>(UpdateTier::HIGH)]
           << " (" << stats.countByTier[static_cast<size_t>(UpdateTier::HIGH)] << " total)\n";
        ss << "  Medium: " << stats.updatesThisFrame[static_cast<size_t>(UpdateTier::MEDIUM)]
           << " (" << stats.countByTier[static_cast<size_t>(UpdateTier::MEDIUM)] << " total)\n";
        ss << "  Low: " << stats.updatesThisFrame[static_cast<size_t>(UpdateTier::LOW)]
           << " (" << stats.countByTier[static_cast<size_t>(UpdateTier::LOW)] << " total)\n";
        ss << "  Minimal: " << stats.updatesThisFrame[static_cast<size_t>(UpdateTier::MINIMAL)]
           << " (" << stats.countByTier[static_cast<size_t>(UpdateTier::MINIMAL)] << " total)\n";
        ss << "  Dormant: " << stats.updatesThisFrame[static_cast<size_t>(UpdateTier::DORMANT)]
           << " (" << stats.countByTier[static_cast<size_t>(UpdateTier::DORMANT)] << " total)\n";
        ss << "Budget Scale: " << std::setprecision(2) << stats.budgetScale << "x\n\n";
    }

    void appendCullingSection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* renderer = subsystems.getRenderingOptimizer();
        if (!renderer) return;

        const auto& stats = renderer->getStats();
        int totalCulled = stats.culledByFrustum + stats.culledByDistance +
                         stats.culledByOcclusion + stats.culledByScreenSize;

        ss << "=== CULLING ===\n";
        ss << "Total Culled: " << totalCulled << "\n";
        ss << "  Frustum: " << stats.culledByFrustum << "\n";
        ss << "  Distance: " << stats.culledByDistance << "\n";
        ss << "  Screen Size: " << stats.culledByScreenSize << "\n";
        ss << "  Occlusion: " << stats.culledByOcclusion << "\n";

        float cullPercent = (stats.totalCreatures > 0) ?
            (totalCulled * 100.0f / stats.totalCreatures) : 0.0f;
        ss << "Cull Rate: " << std::fixed << std::setprecision(1) << cullPercent << "%\n";
        ss << "Cull Time: " << stats.cullTimeMs << " ms\n\n";
    }

    void appendBatchingSection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* renderer = subsystems.getRenderingOptimizer();
        if (!renderer) return;

        const auto& stats = renderer->getStats();

        ss << "=== RENDERING ===\n";
        ss << "Draw Calls: " << stats.drawCalls << "\n";
        ss << "Total Batches: " << stats.totalBatches << "\n";
        ss << "Total Instances: " << stats.totalInstances << "\n";

        float avgInstancesPerBatch = (stats.totalBatches > 0) ?
            (static_cast<float>(stats.totalInstances) / stats.totalBatches) : 0.0f;
        ss << "Avg Instances/Batch: " << std::fixed << std::setprecision(1)
           << avgInstancesPerBatch << "\n";

        ss << "Batch Time: " << stats.batchTimeMs << " ms\n";
        ss << "Sort Time: " << stats.sortTimeMs << " ms\n\n";
    }

    void appendMemorySection(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* renderer = subsystems.getRenderingOptimizer();
        const auto* scaler = subsystems.getQualityScaler();
        if (!renderer || !scaler) return;

        const auto& rStats = renderer->getStats();
        const auto& settings = scaler->getCurrentSettings();

        ss << "=== MEMORY ===\n";

        // Estimate creature memory (rough calculation)
        size_t creatureMemoryMB = (rStats.totalCreatures * 512) / (1024 * 1024);  // ~512 bytes per creature
        ss << "Creatures: ~" << creatureMemoryMB << " MB\n";

        // Instance buffer memory
        size_t instanceMemoryMB = (rStats.totalInstances * 96) / (1024 * 1024);  // 96 bytes per instance
        ss << "Instance Buffers: ~" << instanceMemoryMB << " MB\n";

        // Particle memory estimate
        size_t particleMemoryMB = (settings.maxParticles * 64) / (1024 * 1024);  // 64 bytes per particle
        ss << "Particles: ~" << particleMemoryMB << " MB (max)\n";

        size_t totalMB = creatureMemoryMB + instanceMemoryMB + particleMemoryMB;
        ss << "Total Estimated: ~" << totalMB << " MB\n\n";
    }

    void appendDetailedStats(std::stringstream& ss, const PerformanceSubsystems& subsystems) const {
        const auto* scheduler = subsystems.getScheduler();
        if (!scheduler) return;

        const auto& stats = scheduler->getStats();

        ss << "=== DETAILED STATS ===\n";
        ss << "Total Time: " << std::fixed << std::setprecision(3) << stats.totalTimeMs << " ms\n";
        ss << "Avg Update Time: " << stats.avgUpdateTimeUs << " us\n";

        ss << "\nTime by Tier:\n";
        for (size_t i = 0; i < static_cast<size_t>(UpdateTier::COUNT); ++i) {
            UpdateTier tier = static_cast<UpdateTier>(i);
            float timeMs = stats.timeByTier[i];
            if (timeMs > 0.001f) {
                ss << "  " << CreatureUpdateScheduler::getTierName(tier)
                   << ": " << timeMs << " ms\n";
            }
        }
        ss << "\n";
    }
};

} // namespace Forge
