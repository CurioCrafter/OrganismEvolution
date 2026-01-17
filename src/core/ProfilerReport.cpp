#include "ProfilerReport.h"
#include "SimulationOrchestrator.h"
#include "PerformanceManager.h"
#include "CreatureManager.h"
#include "CreatureUpdateScheduler.h"
#include "MemoryOptimizer.h"
#include "QualityScaler.h"
#include "../graphics/RenderingOptimizer.h"
#include <sstream>
#include <iomanip>

namespace Forge {

// ============================================================================
// Report Generation
// ============================================================================

void ProfilerReport::generateReport(const SimulationOrchestrator* orchestrator,
                                     const PerformanceManager* perfManager,
                                     const CreatureManager* creatureManager,
                                     const CreatureUpdateScheduler* scheduler,
                                     const RenderingOptimizer* renderOptimizer,
                                     const MemoryOptimizer* memOptimizer,
                                     const QualityScaler* qualityScaler) {
    // Gather creature stats
    if (creatureManager) {
        const auto& stats = creatureManager->getStats();
        m_currentReport.totalCreatures = stats.alive;
    }

    if (perfManager) {
        const auto& stats = perfManager->getStats();
        m_currentReport.visibleCreatures = stats.visibleCreatures;
        m_currentReport.drawCalls = stats.drawCalls;
        m_currentReport.triangles = stats.trianglesRendered;
        m_currentReport.instances = stats.instancesRendered;
    }

    if (scheduler) {
        const auto& stats = scheduler->getStats();
        m_currentReport.updatedCreatures = stats.totalUpdates;
    }

    if (memOptimizer) {
        const auto& stats = memOptimizer->getStats();
        m_currentReport.cpuMemory = stats.totalPoolMemory + stats.totalArenaMemory;
        m_currentReport.gpuMemory = stats.estimatedGPUMemory;
    }

    if (renderOptimizer) {
        const auto& stats = renderOptimizer->getStats();
        m_currentReport.drawCalls = stats.drawCalls;
        m_currentReport.instances = stats.totalInstances;
    }
}

// ============================================================================
// Bottleneck Analysis
// ============================================================================

std::string ProfilerReport::analyzeBottlenecks() const {
    std::stringstream ss;
    ss << "=== Bottleneck Analysis ===\n";

    float totalMs = m_currentReport.totalMs;
    if (totalMs < 0.001f) {
        ss << "No frame data available\n";
        return ss.str();
    }

    // Find top consumers
    std::vector<std::pair<ProfileSection, float>> consumers;
    for (int i = 0; i < static_cast<int>(ProfileSection::COUNT); ++i) {
        float ms = m_currentReport.sectionTimes[i];
        if (ms > 0.1f) {  // Only consider sections > 0.1ms
            consumers.push_back({static_cast<ProfileSection>(i), ms});
        }
    }

    std::sort(consumers.begin(), consumers.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    ss << "\nTop Time Consumers:\n";
    for (size_t i = 0; i < std::min(consumers.size(), size_t(5)); ++i) {
        float pct = (consumers[i].second / totalMs) * 100.0f;
        ss << "  " << getSectionName(consumers[i].first)
           << ": " << std::fixed << std::setprecision(2) << consumers[i].second
           << " ms (" << std::setprecision(1) << pct << "%)\n";
    }

    // Identify primary bottleneck
    float updateTotal = m_sections[static_cast<size_t>(ProfileSection::UPDATE_TOTAL)].currentMs;
    float renderTotal = m_sections[static_cast<size_t>(ProfileSection::RENDER_TOTAL)].currentMs;

    ss << "\nPrimary Bottleneck: ";
    if (updateTotal > renderTotal * 1.5f) {
        ss << "CPU/Update (" << std::setprecision(2) << updateTotal << " ms)\n";
        ss << "  Update is significantly slower than rendering.\n";
        ss << "  Consider reducing creature update frequency or using GPU compute.\n";
    } else if (renderTotal > updateTotal * 1.5f) {
        ss << "GPU/Render (" << std::setprecision(2) << renderTotal << " ms)\n";
        ss << "  Rendering is significantly slower than update.\n";
        ss << "  Consider reducing visual quality or draw calls.\n";
    } else {
        ss << "Balanced (Update: " << std::setprecision(2) << updateTotal
           << " ms, Render: " << renderTotal << " ms)\n";
    }

    return ss.str();
}

// ============================================================================
// Optimization Suggestions
// ============================================================================

std::vector<std::string> ProfilerReport::getOptimizationSuggestions() const {
    std::vector<std::string> suggestions;

    float targetBudget = m_config.frameTimeBudgetMs;
    float totalMs = m_currentReport.totalMs;

    // General FPS issues
    if (m_avgFPS < m_config.lowFPSThreshold) {
        suggestions.push_back("Enable adaptive quality scaling to automatically reduce quality when FPS drops.");
    }

    // Creature updates
    float creatureUpdateMs = m_sections[static_cast<size_t>(ProfileSection::UPDATE_CREATURES)].avgMs;
    if (creatureUpdateMs > targetBudget * 0.3f) {
        suggestions.push_back("Creature update is using >30% of frame budget. Consider:");
        suggestions.push_back("  - Increase update tier distances to update fewer creatures per frame");
        suggestions.push_back("  - Use GPU compute for behavior calculations");
        suggestions.push_back("  - Simplify neural network evaluations for distant creatures");
    }

    // Behaviors
    float behaviorMs = m_sections[static_cast<size_t>(ProfileSection::UPDATE_BEHAVIORS)].avgMs;
    if (behaviorMs > targetBudget * 0.2f) {
        suggestions.push_back("Behavior calculation is expensive. Consider:");
        suggestions.push_back("  - Offload flocking calculations to GPU compute shader");
        suggestions.push_back("  - Use spatial grid queries more aggressively");
        suggestions.push_back("  - Reduce neighbor search radius for distant creatures");
    }

    // Flocking specifically
    float flockingMs = m_sections[static_cast<size_t>(ProfileSection::UPDATE_FLOCKING)].avgMs;
    if (flockingMs > targetBudget * 0.15f) {
        suggestions.push_back("Flocking is a hotspot. The GPU flocking compute shader should be enabled.");
    }

    // Rendering
    float renderCreaturesMs = m_sections[static_cast<size_t>(ProfileSection::RENDER_CREATURES)].avgMs;
    if (renderCreaturesMs > targetBudget * 0.25f) {
        suggestions.push_back("Creature rendering is expensive. Consider:");
        suggestions.push_back("  - Enable instanced rendering for creatures at same LOD");
        suggestions.push_back("  - Increase LOD bias to use simpler meshes");
        suggestions.push_back("  - Use billboard sprites for distant creatures");
    }

    // Draw calls
    if (m_currentReport.drawCalls > m_config.maxDrawCallsWarning) {
        suggestions.push_back("High draw call count. Consider:");
        suggestions.push_back("  - Batch creatures by type and LOD level");
        suggestions.push_back("  - Use texture atlases to reduce material switches");
        suggestions.push_back("  - Enable mesh instancing");
    }

    // Particles
    float particleMs = m_sections[static_cast<size_t>(ProfileSection::RENDER_PARTICLES)].avgMs;
    if (particleMs > targetBudget * 0.1f) {
        suggestions.push_back("Particle rendering is slow. Consider:");
        suggestions.push_back("  - Reduce max particle count");
        suggestions.push_back("  - Use simpler particle shaders");
        suggestions.push_back("  - Cull particle emitters by distance");
    }

    // Spatial grid
    float spatialGridMs = m_sections[static_cast<size_t>(ProfileSection::SPATIAL_GRID)].avgMs;
    if (spatialGridMs > targetBudget * 0.05f) {
        suggestions.push_back("Spatial grid operations are slow. Consider:");
        suggestions.push_back("  - Increase grid resolution for finer cells");
        suggestions.push_back("  - Use hierarchical grid for variable-scale queries");
        suggestions.push_back("  - Cache frequent query results");
    }

    // Memory
    if (m_currentReport.cpuMemory > m_config.memoryWarningMB * 1024 * 1024) {
        suggestions.push_back("High memory usage. Consider:");
        suggestions.push_back("  - Enable object pooling to reduce allocations");
        suggestions.push_back("  - Compact spatial grids periodically");
        suggestions.push_back("  - Stream distant terrain and vegetation");
    }

    if (suggestions.empty()) {
        suggestions.push_back("Performance is within acceptable bounds. No major issues detected.");
    }

    return suggestions;
}

// ============================================================================
// GPU/CPU Bound Detection
// ============================================================================

bool ProfilerReport::isGPUBound() const {
    float renderTotal = m_sections[static_cast<size_t>(ProfileSection::RENDER_TOTAL)].avgMs;
    float updateTotal = m_sections[static_cast<size_t>(ProfileSection::UPDATE_TOTAL)].avgMs;

    // GPU bound if render time is significantly higher
    return renderTotal > updateTotal * 1.3f;
}

bool ProfilerReport::isCPUBound() const {
    float renderTotal = m_sections[static_cast<size_t>(ProfileSection::RENDER_TOTAL)].avgMs;
    float updateTotal = m_sections[static_cast<size_t>(ProfileSection::UPDATE_TOTAL)].avgMs;

    // CPU bound if update time is significantly higher
    return updateTotal > renderTotal * 1.3f;
}

// ============================================================================
// Export
// ============================================================================

std::string ProfilerReport::exportToString() const {
    std::stringstream ss;

    ss << "=== Performance Report ===\n";
    ss << "Frame: " << m_currentReport.frameNumber << "\n";
    ss << std::fixed << std::setprecision(2);
    ss << "FPS: " << m_currentReport.fps << " (Avg: " << m_avgFPS << ")\n";
    ss << "Frame Time: " << m_currentReport.totalMs << " ms\n";
    ss << "\n";

    ss << "Creatures:\n";
    ss << "  Total: " << m_currentReport.totalCreatures << "\n";
    ss << "  Visible: " << m_currentReport.visibleCreatures << "\n";
    ss << "  Updated: " << m_currentReport.updatedCreatures << "\n";
    ss << "\n";

    ss << "Rendering:\n";
    ss << "  Draw Calls: " << m_currentReport.drawCalls << "\n";
    ss << "  Triangles: " << m_currentReport.triangles << "\n";
    ss << "  Instances: " << m_currentReport.instances << "\n";
    ss << "\n";

    ss << "Section Times:\n";
    for (int i = 0; i < static_cast<int>(ProfileSection::COUNT); ++i) {
        float ms = m_currentReport.sectionTimes[i];
        if (ms > 0.01f) {
            ss << "  " << getSectionName(static_cast<ProfileSection>(i))
               << ": " << ms << " ms\n";
        }
    }

    ss << "\n";
    ss << analyzeBottlenecks();

    return ss.str();
}

std::string ProfilerReport::exportToCSV() const {
    std::stringstream ss;

    ss << m_currentReport.frameNumber << ","
       << std::fixed << std::setprecision(3)
       << m_currentReport.fps << ","
       << m_currentReport.totalMs << ","
       << m_currentReport.totalCreatures << ","
       << m_currentReport.visibleCreatures << ","
       << m_currentReport.updatedCreatures << ","
       << m_currentReport.drawCalls << ","
       << m_currentReport.triangles;

    for (int i = 0; i < static_cast<int>(ProfileSection::COUNT); ++i) {
        ss << "," << m_currentReport.sectionTimes[i];
    }

    return ss.str();
}

} // namespace Forge
