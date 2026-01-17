#include "PerformanceUI.h"
#include "../core/PerformanceManager.h"
#include "../core/QualityScaler.h"
#include "../core/MemoryOptimizer.h"
#include "../core/CreatureUpdateScheduler.h"
#include "../core/CreatureManager.h"
#include "../graphics/RenderingOptimizer.h"

namespace Forge {

// ============================================================================
// Main Render
// ============================================================================

void PerformanceUI::render(const PerformanceManager* perfManager,
                            QualityScaler* qualityScaler,
                            const MemoryOptimizer* memOptimizer,
                            const CreatureUpdateScheduler* scheduler,
                            const RenderingOptimizer* renderOptimizer,
                            const CreatureManager* creatureManager) {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Performance Monitor", &m_visible)) {
        // Tab bar for different sections
        if (ImGui::BeginTabBar("PerfTabs")) {
            // Overview Tab
            if (ImGui::BeginTabItem("Overview")) {
                if (m_config.showFPSGraph && perfManager) {
                    renderFPSSection(perfManager);
                }
                if (m_config.showFrameTimeGraph && perfManager) {
                    renderFrameTimeSection(perfManager);
                }
                ImGui::EndTabItem();
            }

            // Creatures Tab
            if (ImGui::BeginTabItem("Creatures")) {
                if (m_config.showCreatureStats) {
                    renderCreatureSection(perfManager, creatureManager, scheduler);
                }
                ImGui::EndTabItem();
            }

            // Rendering Tab
            if (ImGui::BeginTabItem("Rendering")) {
                if (m_config.showRenderingStats && renderOptimizer) {
                    renderRenderingSection(renderOptimizer, perfManager);
                }
                ImGui::EndTabItem();
            }

            // Memory Tab
            if (ImGui::BeginTabItem("Memory")) {
                if (m_config.showMemoryGraph && memOptimizer) {
                    renderMemorySection(memOptimizer);
                }
                ImGui::EndTabItem();
            }

            // Quality Tab
            if (ImGui::BeginTabItem("Quality")) {
                if (m_config.showQualitySettings && qualityScaler) {
                    renderQualitySection(qualityScaler);
                }
                ImGui::EndTabItem();
            }

            // Breakdown Tab
            if (ImGui::BeginTabItem("Breakdown")) {
                if (m_config.showSystemBreakdown && perfManager) {
                    renderSystemBreakdown(perfManager);
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

// ============================================================================
// Minimal Overlay
// ============================================================================

void PerformanceUI::renderOverlay(const PerformanceManager* perfManager) {
    if (!perfManager) return;

    const auto& stats = perfManager->getStats();
    float fps = stats.currentFPS;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                              ImGuiWindowFlags_NoInputs |
                              ImGuiWindowFlags_AlwaysAutoResize |
                              ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoFocusOnAppearing |
                              ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin("FPS Overlay", nullptr, flags)) {
        ImVec4 color = getFPSColor(fps);
        ImGui::TextColored(color, "FPS: %.1f", fps);
        ImGui::Text("Frame: %.2f ms", stats.frameTime);
        ImGui::Text("Creatures: %d", stats.totalCreatures);
    }
    ImGui::End();
}

// ============================================================================
// Section Renderers
// ============================================================================

void PerformanceUI::renderFPSSection(const PerformanceManager* perfManager) {
    const auto& stats = perfManager->getStats();

    ImGui::SeparatorText("Frame Rate");

    // FPS display with color
    ImVec4 fpsColor = getFPSColor(stats.currentFPS);
    ImGui::TextColored(fpsColor, "Current FPS: %.1f", stats.currentFPS);

    ImGui::SameLine();
    ImGui::Text("(Avg: %.1f, Min: %.1f, Max: %.1f)",
                stats.avgFPS, stats.minFPS, stats.maxFPS);

    // FPS graph
    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%.1f FPS", m_fpsGraph.last());
    drawPlotLines("##FPS", m_fpsGraph, m_config.graphHeight, overlay);

    // Target line indicator
    ImGui::Text("Target: 60 FPS | Budget: 16.67 ms");
}

void PerformanceUI::renderFrameTimeSection(const PerformanceManager* perfManager) {
    const auto& stats = perfManager->getStats();

    ImGui::SeparatorText("Frame Time");

    // Frame time breakdown
    ImGui::Text("Total: %.2f ms", stats.frameTime);

    // Progress bar showing budget usage
    float budgetUsage = stats.frameTime / 16.67f;
    ImVec4 barColor = budgetUsage > 1.0f ? m_config.badColor :
                      budgetUsage > 0.9f ? m_config.warningColor : m_config.goodColor;

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(std::min(budgetUsage, 1.5f) / 1.5f, ImVec2(-1, 0),
                       budgetUsage > 1.0f ? "Over Budget!" : "Within Budget");
    ImGui::PopStyleColor();

    // Frame time graph
    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%.2f ms", m_frameTimeGraph.last());
    drawPlotLines("##FrameTime", m_frameTimeGraph, m_config.graphHeight, overlay);
}

void PerformanceUI::renderCreatureSection(const PerformanceManager* perfManager,
                                           const CreatureManager* creatureManager,
                                           const CreatureUpdateScheduler* scheduler) {
    ImGui::SeparatorText("Creature Statistics");

    if (creatureManager) {
        const auto& popStats = creatureManager->getStats();
        ImGui::Text("Total Population: %d", popStats.alive);
        ImGui::Text("Births: %d | Deaths: %d", popStats.births, popStats.deaths);
        ImGui::Text("Avg Energy: %.1f | Avg Age: %.1f",
                    popStats.avgEnergy, popStats.avgAge);
    }

    if (perfManager) {
        const auto& stats = perfManager->getStats();

        ImGui::Separator();
        ImGui::Text("LOD Distribution:");

        // LOD bar chart
        const char* lodNames[] = {"Full", "Medium", "Low", "Billboard", "Culled"};
        int total = stats.totalCreatures;

        for (int i = 0; i < 5; ++i) {
            int count = stats.creaturesByLOD[i];
            float pct = total > 0 ? (float)count / total : 0.0f;

            ImGui::Text("%s: %d", lodNames[i], count);
            ImGui::SameLine(100);
            ImGui::ProgressBar(pct, ImVec2(150, 0), "");
        }

        ImGui::Separator();
        ImGui::Text("Culling:");
        ImGui::Text("  Visible: %d", stats.visibleCreatures);
        ImGui::Text("  Frustum Culled: %d", stats.culledByFrustum);
        ImGui::Text("  Distance Culled: %d", stats.culledByDistance);
    }

    if (scheduler) {
        const auto& schedStats = scheduler->getStats();

        ImGui::Separator();
        ImGui::Text("Update Scheduling:");
        ImGui::Text("  Updates This Frame: %d / %d",
                    schedStats.totalUpdates, schedStats.totalCreatures);
        ImGui::Text("  Update Rate: %.1f%%", schedStats.updateRate * 100.0f);
        ImGui::Text("  Avg Update Time: %.1f us", schedStats.avgUpdateTimeUs);

        // Tier distribution
        ImGui::Text("Update Tiers:");
        for (int i = 0; i < static_cast<int>(UpdateTier::COUNT); ++i) {
            ImGui::Text("  %s: %d (updated: %d)",
                        CreatureUpdateScheduler::getTierName(static_cast<UpdateTier>(i)),
                        schedStats.countByTier[i],
                        schedStats.updatesThisFrame[i]);
        }
    }
}

void PerformanceUI::renderRenderingSection(const RenderingOptimizer* renderOptimizer,
                                            const PerformanceManager* perfManager) {
    ImGui::SeparatorText("Rendering Statistics");

    if (renderOptimizer) {
        const auto& stats = renderOptimizer->getStats();

        ImGui::Text("Visible Creatures: %d / %d",
                    stats.visibleCreatures, stats.totalCreatures);

        ImGui::Separator();
        ImGui::Text("LOD Counts:");
        for (int i = 0; i < static_cast<int>(MeshLOD::COUNT); ++i) {
            ImGui::Text("  %s: %d",
                        RenderingOptimizer::getLODName(static_cast<MeshLOD>(i)),
                        stats.countByLOD[i]);
        }

        ImGui::Separator();
        ImGui::Text("Batching:");
        ImGui::Text("  Draw Calls: %d", stats.drawCalls);
        ImGui::Text("  Total Batches: %d", stats.totalBatches);
        ImGui::Text("  Total Instances: %d", stats.totalInstances);

        ImGui::Separator();
        ImGui::Text("Timing:");
        ImGui::Text("  Cull Time: %.2f ms", stats.cullTimeMs);
        ImGui::Text("  Sort Time: %.2f ms", stats.sortTimeMs);
        ImGui::Text("  Batch Time: %.2f ms", stats.batchTimeMs);
    }

    if (perfManager) {
        const auto& stats = perfManager->getStats();

        ImGui::Separator();
        ImGui::Text("Render Metrics:");
        ImGui::Text("  Triangles: %d", stats.trianglesRendered);
        ImGui::Text("  Instances: %d", stats.instancesRendered);
    }
}

void PerformanceUI::renderMemorySection(const MemoryOptimizer* memOptimizer) {
    ImGui::SeparatorText("Memory Usage");

    if (memOptimizer) {
        const auto& stats = memOptimizer->getStats();

        // Arena usage
        ImGui::Text("Frame Arena:");
        float arenaUsage = stats.arenaTotal > 0 ?
                           (float)stats.arenaUsed / stats.arenaTotal : 0.0f;
        ImGui::ProgressBar(arenaUsage, ImVec2(-1, 0));
        ImGui::Text("  Used: %.2f MB / %.2f MB",
                    stats.arenaUsed / (1024.0f * 1024.0f),
                    stats.arenaTotal / (1024.0f * 1024.0f));

        ImGui::Separator();
        ImGui::Text("Pool Statistics:");
        ImGui::Text("  Creature Pool: %zu / %zu",
                    stats.creaturePoolActive, stats.creaturePoolTotal);
        ImGui::Text("  Particle Pool: %zu / %zu",
                    stats.particlePoolActive, stats.particlePoolTotal);

        ImGui::Separator();
        ImGui::Text("Per-Frame Allocations: %zu", stats.frameAllocations);
        if (stats.frameAllocations > 100) {
            ImGui::SameLine();
            ImGui::TextColored(m_config.warningColor, "(High!)");
        }

        ImGui::Separator();
        ImGui::Text("Total Memory:");
        ImGui::Text("  Pool Memory: %.2f MB",
                    stats.totalPoolMemory / (1024.0f * 1024.0f));
        ImGui::Text("  Arena Memory: %.2f MB",
                    stats.totalArenaMemory / (1024.0f * 1024.0f));
        ImGui::Text("  GPU Memory: %.2f MB",
                    stats.estimatedGPUMemory / (1024.0f * 1024.0f));
    }
}

void PerformanceUI::renderQualitySection(QualityScaler* qualityScaler) {
    ImGui::SeparatorText("Quality Settings");

    if (!qualityScaler) return;

    const auto& stats = qualityScaler->getStats();
    const auto& settings = qualityScaler->getCurrentSettings();

    // Current preset
    ImGui::Text("Current Preset: %s", QualityScaler::getPresetName(stats.currentPreset));
    ImGui::Text("Quality Level: %.1f%%", stats.qualityLevel * 100.0f);
    ImGui::Text("Preset Changes: %d", stats.presetChanges);

    ImGui::Separator();

    // Auto-adjust toggle
    bool autoAdjust = qualityScaler->isAutoAdjust();
    if (ImGui::Checkbox("Auto-Adjust Quality", &autoAdjust)) {
        qualityScaler->setAutoAdjust(autoAdjust);
    }

    // Manual preset selection
    if (!autoAdjust) {
        ImGui::Text("Manual Preset:");
        static int presetIdx = static_cast<int>(stats.currentPreset);

        const char* presetNames[] = {"Ultra Low", "Low", "Medium", "High", "Ultra"};
        if (ImGui::Combo("##Preset", &presetIdx, presetNames, 5)) {
            qualityScaler->setPreset(static_cast<QualityPreset>(presetIdx));
        }
    }

    ImGui::Separator();
    ImGui::Text("Current Settings:");

    // Display key settings
    ImGui::BulletText("Max Creatures: %d", settings.maxCreatures);
    ImGui::BulletText("Max Visible: %d", settings.maxVisibleCreatures);
    ImGui::BulletText("Max Particles: %d", settings.maxParticles);
    ImGui::BulletText("LOD Bias: %.2f", settings.lodBias);
    ImGui::BulletText("Shadow Res: %d", settings.shadowResolution);
    ImGui::BulletText("Shadow Distance: %.0f", settings.shadowDistance);

    // Feature toggles
    ImGui::Separator();
    ImGui::Text("Features:");
    ImGui::BulletText("Shadows: %s", settings.enableShadows ? "ON" : "OFF");
    ImGui::BulletText("SSAO: %s", settings.enableSSAO ? "ON" : "OFF");
    ImGui::BulletText("Bloom: %s", settings.enableBloom ? "ON" : "OFF");
    ImGui::BulletText("Water Reflections: %s", settings.enableWaterReflections ? "ON" : "OFF");
    ImGui::BulletText("Grass Animation: %s", settings.enableGrassAnimation ? "ON" : "OFF");
}

void PerformanceUI::renderSystemBreakdown(const PerformanceManager* perfManager) {
    ImGui::SeparatorText("System Time Breakdown");

    if (!perfManager) return;

    // Get section times
    float creatureUpdate = perfManager->getSectionTime("CreatureUpdate");
    float behaviorCalc = perfManager->getSectionTime("BehaviorCalc");
    float rendering = perfManager->getSectionTime("Rendering");
    float particles = perfManager->getSectionTime("Particles");
    float ui = perfManager->getSectionTime("UI");
    float physics = perfManager->getSectionTime("Physics");
    float audio = perfManager->getSectionTime("Audio");

    float total = creatureUpdate + behaviorCalc + rendering + particles + ui + physics + audio;
    total = std::max(total, 0.001f);  // Avoid division by zero

    // Bar chart
    auto drawBar = [&](const char* name, float time, ImVec4 color) {
        float pct = time / total;
        ImGui::Text("%-15s %6.2f ms (%5.1f%%)", name, time, pct * 100.0f);
        ImGui::SameLine(250);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(pct, ImVec2(100, 0), "");
        ImGui::PopStyleColor();
    };

    drawBar("Creature Update", creatureUpdate, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
    drawBar("Behavior", behaviorCalc, ImVec4(0.9f, 0.6f, 0.2f, 1.0f));
    drawBar("Rendering", rendering, ImVec4(0.2f, 0.9f, 0.4f, 1.0f));
    drawBar("Particles", particles, ImVec4(0.9f, 0.2f, 0.6f, 1.0f));
    drawBar("Physics", physics, ImVec4(0.6f, 0.2f, 0.9f, 1.0f));
    drawBar("Audio", audio, ImVec4(0.2f, 0.9f, 0.9f, 1.0f));
    drawBar("UI", ui, ImVec4(0.9f, 0.9f, 0.2f, 1.0f));

    ImGui::Separator();
    ImGui::Text("Total Tracked: %.2f ms", total);

    // Update/Render time graphs
    ImGui::Separator();
    ImGui::Text("Update Time History:");
    drawPlotLines("##UpdateTime", m_updateTimeGraph, m_config.graphHeight, nullptr);

    ImGui::Text("Render Time History:");
    drawPlotLines("##RenderTime", m_renderTimeGraph, m_config.graphHeight, nullptr);
}

} // namespace Forge
