#include "IslandComparisonUI.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

IslandComparisonUI::IslandComparisonUI() {
    // Initialize population history for up to MAX_ISLANDS
    m_populationHistory.resize(MultiIslandManager::MAX_ISLANDS);
}

// ============================================================================
// Main Render
// ============================================================================

void IslandComparisonUI::render(const MultiIslandManager& islands) {
    render(islands, nullptr);
}

void IslandComparisonUI::render(const MultiIslandManager& islands, const InterIslandMigration* migration) {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(m_config.panelWidth, m_config.panelHeight), ImGuiCond_FirstUseEver);

    ImGui::Begin("Island Comparison", &m_visible);

    // Mode selector tabs
    if (ImGui::BeginTabBar("ComparisonModes")) {
        if (ImGui::BeginTabItem("Overview")) {
            m_mode = ComparisonMode::OVERVIEW;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Side-by-Side")) {
            m_mode = ComparisonMode::SIDE_BY_SIDE;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Gene Flow")) {
            m_mode = ComparisonMode::GENE_FLOW;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Divergence")) {
            m_mode = ComparisonMode::DIVERGENCE_TREE;
            ImGui::EndTabItem();
        }
        if (migration && ImGui::BeginTabItem("Migrations")) {
            m_mode = ComparisonMode::MIGRATION_MAP;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    // Render based on mode
    switch (m_mode) {
        case ComparisonMode::OVERVIEW:
            renderOverview(islands);
            break;
        case ComparisonMode::SIDE_BY_SIDE:
            renderSideBySide(islands);
            break;
        case ComparisonMode::GENE_FLOW:
            renderGeneFlowDiagram(islands, migration);
            break;
        case ComparisonMode::DIVERGENCE_TREE:
            renderDivergenceTree(islands);
            break;
        case ComparisonMode::MIGRATION_MAP:
            renderMigrationMap(islands, migration);
            break;
    }

    ImGui::End();
}

// ============================================================================
// Overview Mode
// ============================================================================

void IslandComparisonUI::renderOverview(const MultiIslandManager& islands) {
    // Global statistics header
    auto globalStats = islands.getGlobalStats();

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Archipelago Overview");
    ImGui::Separator();

    // Global stats summary
    ImGui::Columns(4, "GlobalStats", false);

    ImGui::Text("Total Creatures");
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "%d", globalStats.totalCreatures);
    ImGui::NextColumn();

    ImGui::Text("Species");
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%d", globalStats.speciesCount);
    ImGui::NextColumn();

    ImGui::Text("Avg Fitness");
    ImGui::TextColored(ImVec4(0.3f, 0.7f, 0.9f, 1.0f), "%.2f", globalStats.avgFitness);
    ImGui::NextColumn();

    ImGui::Text("Diversity");
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.9f, 1.0f), "%.1f%%", globalStats.geneticDiversity * 100.0f);
    ImGui::NextColumn();

    ImGui::Columns(1);

    // Migration summary (if any migrations occurred)
    if (globalStats.immigrations > 0 || globalStats.emigrations > 0) {
        ImGui::Spacing();
        ImGui::Text("Total Migrations:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.9f, 1.0f), "%d arrivals, %d departures",
                          globalStats.immigrations, globalStats.emigrations);
    }

    ImGui::Separator();

    // Island cards
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Islands (%d)", islands.getIslandCount());

    // Scrollable list of islands
    ImGui::BeginChild("IslandList", ImVec2(0, 0), true);

    for (uint32_t i = 0; i < islands.getIslandCount(); ++i) {
        const auto* island = islands.getIsland(i);
        if (!island) continue;

        renderIslandCard(*island, i);

        if (i < islands.getIslandCount() - 1) {
            ImGui::Separator();
        }
    }

    ImGui::EndChild();
}

void IslandComparisonUI::renderIslandCard(const Island& island, uint32_t index) {
    glm::vec4 color = getIslandColor(index);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, 0.5f));

    bool isActive = island.isActive;
    std::string header = island.name;
    if (isActive) {
        header += " [ACTIVE]";
    }

    if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Basic stats
        ImGui::Columns(3, nullptr, false);

        ImGui::Text("Population");
        ImGui::TextColored(ImVec4(color.r, color.g, color.b, 1.0f), "%d", island.stats.totalCreatures);
        ImGui::NextColumn();

        ImGui::Text("Species");
        ImGui::Text("%d", island.stats.speciesCount);
        ImGui::NextColumn();

        ImGui::Text("Fitness");
        ImGui::Text("%.2f", island.stats.avgFitness);
        ImGui::NextColumn();

        ImGui::Columns(1);

        // Mini progress bars
        float diversity = island.stats.geneticDiversity;
        float energyNorm = island.stats.avgEnergy / 200.0f;

        ImGui::Text("Genetic Diversity");
        ImGui::ProgressBar(diversity, ImVec2(-1, 0), "");

        ImGui::Text("Avg Energy");
        ImGui::ProgressBar(energyNorm, ImVec2(-1, 0), "");

        // Migration stats
        if (island.stats.immigrations > 0 || island.stats.emigrations > 0) {
            ImGui::Text("Migrations: +%d / -%d", island.stats.immigrations, island.stats.emigrations);
        }

        ImGui::Unindent();
    }

    ImGui::PopStyleColor();
}

// ============================================================================
// Side-by-Side Mode
// ============================================================================

void IslandComparisonUI::renderSideBySide(const MultiIslandManager& islands) {
    if (islands.getIslandCount() < 2) {
        ImGui::Text("Need at least 2 islands for comparison");
        return;
    }

    // Island selectors
    ImGui::Columns(2, "IslandSelectors", true);

    // Island A selector
    ImGui::Text("Island A:");
    if (ImGui::BeginCombo("##IslandA", islands.getIsland(m_selectedIslandA)->name.c_str())) {
        for (uint32_t i = 0; i < islands.getIslandCount(); ++i) {
            bool selected = (i == m_selectedIslandA);
            if (ImGui::Selectable(islands.getIsland(i)->name.c_str(), selected)) {
                m_selectedIslandA = i;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::NextColumn();

    // Island B selector
    ImGui::Text("Island B:");
    if (ImGui::BeginCombo("##IslandB", islands.getIsland(m_selectedIslandB)->name.c_str())) {
        for (uint32_t i = 0; i < islands.getIslandCount(); ++i) {
            bool selected = (i == m_selectedIslandB);
            if (ImGui::Selectable(islands.getIsland(i)->name.c_str(), selected)) {
                m_selectedIslandB = i;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Columns(1);
    ImGui::Separator();

    // Get islands
    const auto* islandA = islands.getIsland(m_selectedIslandA);
    const auto* islandB = islands.getIsland(m_selectedIslandB);

    if (!islandA || !islandB) return;

    // Genetic distance between islands
    float geneticDistance = islands.getGeneticDistance(m_selectedIslandA, m_selectedIslandB);
    float physicalDistance = islands.getIslandDistance(m_selectedIslandA, m_selectedIslandB);

    ImGui::Text("Genetic Distance: %.3f", geneticDistance);
    ImGui::SameLine();
    ImGui::Text("Physical Distance: %.0f units", physicalDistance);

    // Divergence indicator
    ImGui::Text("Divergence Status: ");
    ImGui::SameLine();
    if (geneticDistance > 0.5f) {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "HIGH - Approaching speciation");
    } else if (geneticDistance > 0.3f) {
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "MODERATE - Diverging");
    } else {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "LOW - Gene flow active");
    }

    ImGui::Separator();

    // Comparison bars
    renderComparisonBars(islandA->stats, islandB->stats);
}

void IslandComparisonUI::renderComparisonBars(const IslandStats& statsA, const IslandStats& statsB) {
    float maxPop = static_cast<float>(std::max(statsA.totalCreatures, statsB.totalCreatures));
    if (maxPop < 1.0f) maxPop = 1.0f;

    ImGui::BeginChild("ComparisonBars", ImVec2(0, 300), true);

    // Population comparison
    ImGui::Text("Population");
    ImGui::Columns(3, "PopCols", false);

    glm::vec4 colorA = getIslandColor(m_selectedIslandA);
    glm::vec4 colorB = getIslandColor(m_selectedIslandB);

    // Left bar (Island A)
    float popANorm = statsA.totalCreatures / maxPop;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(colorA.r, colorA.g, colorA.b, 1.0f));
    ImGui::ProgressBar(popANorm, ImVec2(-1, 20), std::to_string(statsA.totalCreatures).c_str());
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("vs");
    ImGui::NextColumn();

    // Right bar (Island B)
    float popBNorm = statsB.totalCreatures / maxPop;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(colorB.r, colorB.g, colorB.b, 1.0f));
    ImGui::ProgressBar(popBNorm, ImVec2(-1, 20), std::to_string(statsB.totalCreatures).c_str());
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Separator();

    // Fitness comparison
    ImGui::Text("Average Fitness");
    float maxFit = std::max(statsA.avgFitness, statsB.avgFitness);
    if (maxFit < 0.1f) maxFit = 1.0f;

    ImGui::Columns(3, "FitCols", false);

    float fitANorm = statsA.avgFitness / maxFit;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(colorA.r, colorA.g, colorA.b, 1.0f));
    char bufA[32];
    snprintf(bufA, sizeof(bufA), "%.2f", statsA.avgFitness);
    ImGui::ProgressBar(fitANorm, ImVec2(-1, 20), bufA);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("vs");
    ImGui::NextColumn();

    float fitBNorm = statsB.avgFitness / maxFit;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(colorB.r, colorB.g, colorB.b, 1.0f));
    char bufB[32];
    snprintf(bufB, sizeof(bufB), "%.2f", statsB.avgFitness);
    ImGui::ProgressBar(fitBNorm, ImVec2(-1, 20), bufB);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Separator();

    // Genetic Diversity comparison
    ImGui::Text("Genetic Diversity");
    ImGui::Columns(3, "DivCols", false);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(colorA.r, colorA.g, colorA.b, 1.0f));
    char divBufA[32];
    snprintf(divBufA, sizeof(divBufA), "%.1f%%", statsA.geneticDiversity * 100.0f);
    ImGui::ProgressBar(statsA.geneticDiversity, ImVec2(-1, 20), divBufA);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("vs");
    ImGui::NextColumn();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(colorB.r, colorB.g, colorB.b, 1.0f));
    char divBufB[32];
    snprintf(divBufB, sizeof(divBufB), "%.1f%%", statsB.geneticDiversity * 100.0f);
    ImGui::ProgressBar(statsB.geneticDiversity, ImVec2(-1, 20), divBufB);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Separator();

    // Species count
    ImGui::Text("Species Count: %d vs %d", statsA.speciesCount, statsB.speciesCount);

    // Migration balance
    int netMigA = statsA.immigrations - statsA.emigrations;
    int netMigB = statsB.immigrations - statsB.emigrations;
    ImGui::Text("Net Migration: %+d vs %+d", netMigA, netMigB);

    ImGui::EndChild();
}

// ============================================================================
// Gene Flow Diagram
// ============================================================================

void IslandComparisonUI::renderGeneFlowDiagram(const MultiIslandManager& islands,
                                                 const InterIslandMigration* migration) {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Gene Flow Between Islands");
    ImGui::Separator();

    // Collect gene flow data
    auto flowEdges = collectGeneFlowData(islands, migration);

    // Render genetic distance matrix
    renderGeneticDistanceMatrix(islands);

    ImGui::Separator();

    // Flow summary
    ImGui::Text("Migration Summary:");

    if (flowEdges.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No migration events recorded");
    } else {
        for (const auto& edge : flowEdges) {
            const auto* fromIsland = islands.getIsland(edge.fromIsland);
            const auto* toIsland = islands.getIsland(edge.toIsland);

            if (fromIsland && toIsland) {
                ImGui::TextColored(ImVec4(edge.color.r, edge.color.g, edge.color.b, 1.0f),
                                   "%s -> %s: %d migrants",
                                   fromIsland->name.c_str(),
                                   toIsland->name.c_str(),
                                   edge.migrationCount);
            }
        }
    }

    // Active migrations
    if (migration) {
        int activeCount = migration->getActiveMigrationCount();
        if (activeCount > 0) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Active Migrations: %d", activeCount);
        }
    }
}

void IslandComparisonUI::renderGeneticDistanceMatrix(const MultiIslandManager& islands) {
    uint32_t count = islands.getIslandCount();
    if (count < 2) return;

    ImGui::Text("Genetic Distance Matrix:");

    // Column headers
    ImGui::Columns(static_cast<int>(count) + 1, "DistMatrix", true);
    ImGui::Text("");  // Empty corner
    ImGui::NextColumn();

    for (uint32_t i = 0; i < count; ++i) {
        const auto* island = islands.getIsland(i);
        if (island) {
            ImGui::Text("%s", island->name.substr(0, 6).c_str());
        }
        ImGui::NextColumn();
    }

    // Matrix rows
    for (uint32_t row = 0; row < count; ++row) {
        const auto* rowIsland = islands.getIsland(row);
        if (rowIsland) {
            ImGui::Text("%s", rowIsland->name.substr(0, 6).c_str());
        }
        ImGui::NextColumn();

        for (uint32_t col = 0; col < count; ++col) {
            if (row == col) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
            } else {
                float dist = islands.getGeneticDistance(row, col);
                glm::vec4 color = getColorForStat(dist, 0.0f, 1.0f);
                ImGui::TextColored(ImVec4(color.r, color.g, color.b, 1.0f), "%.2f", dist);
            }
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
}

// ============================================================================
// Divergence Tree
// ============================================================================

void IslandComparisonUI::renderDivergenceTree(const MultiIslandManager& islands) {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Evolutionary Divergence");
    ImGui::Separator();

    // Simplified tree visualization
    // In a full implementation, this would render an actual phylogenetic tree

    uint32_t count = islands.getIslandCount();

    // Calculate and display divergence from first island as reference
    ImGui::Text("Divergence from %s (reference):",
                islands.getIsland(0) ? islands.getIsland(0)->name.c_str() : "Island 0");

    for (uint32_t i = 1; i < count; ++i) {
        float dist = islands.getGeneticDistance(0, i);
        const auto* island = islands.getIsland(i);

        if (island) {
            // Visual divergence bar
            glm::vec4 color = getIslandColor(i);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(color.r, color.g, color.b, 1.0f));

            char label[64];
            snprintf(label, sizeof(label), "%s (%.3f)", island->name.c_str(), dist);
            ImGui::ProgressBar(dist, ImVec2(-1, 20), label);

            ImGui::PopStyleColor();
        }
    }

    ImGui::Separator();

    // Speciation predictions
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Speciation Status:");

    for (uint32_t i = 0; i < count; ++i) {
        for (uint32_t j = i + 1; j < count; ++j) {
            float dist = islands.getGeneticDistance(i, j);

            const auto* islandI = islands.getIsland(i);
            const auto* islandJ = islands.getIsland(j);

            if (dist > 0.7f && islandI && islandJ) {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
                                   "! %s and %s may be undergoing speciation",
                                   islandI->name.c_str(), islandJ->name.c_str());
            }
        }
    }
}

// ============================================================================
// Migration Map
// ============================================================================

void IslandComparisonUI::renderMigrationMap(const MultiIslandManager& islands,
                                             const InterIslandMigration* migration) {
    if (!migration) {
        ImGui::Text("Migration system not available");
        return;
    }

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Active Migrations");
    ImGui::Separator();

    const auto& activeMigrations = migration->getActiveMigrations();
    const auto& stats = migration->getStats();

    // Summary stats
    ImGui::Columns(4, "MigStats", false);

    ImGui::Text("Total Attempts");
    ImGui::Text("%d", stats.totalAttempts);
    ImGui::NextColumn();

    ImGui::Text("Successful");
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "%d", stats.successfulMigrations);
    ImGui::NextColumn();

    ImGui::Text("Failed");
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%d", stats.failedMigrations);
    ImGui::NextColumn();

    ImGui::Text("In Progress");
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%d", stats.inProgressMigrations);
    ImGui::NextColumn();

    ImGui::Columns(1);

    // Survival rate
    ImGui::Text("Overall Survival Rate: %.1f%%", stats.avgSurvivalRate * 100.0f);
    ImGui::ProgressBar(stats.avgSurvivalRate, ImVec2(-1, 0), "");

    ImGui::Separator();

    // Active migrations list
    if (activeMigrations.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active migrations");
    } else {
        ImGui::BeginChild("ActiveMigrations", ImVec2(0, 200), true);

        for (const auto& event : activeMigrations) {
            const auto* srcIsland = islands.getIsland(event.sourceIsland);
            const auto* dstIsland = islands.getIsland(event.targetIsland);

            if (srcIsland && dstIsland) {
                // Migration progress bar
                ImGui::Text("%s -> %s", srcIsland->name.c_str(), dstIsland->name.c_str());

                // State indicator
                const char* stateStr = "";
                ImVec4 stateColor = ImVec4(1, 1, 1, 1);
                switch (event.state) {
                    case MigrationState::INITIATING:
                        stateStr = "Starting";
                        stateColor = ImVec4(0.3f, 0.3f, 0.9f, 1.0f);
                        break;
                    case MigrationState::IN_TRANSIT:
                        stateStr = "In Transit";
                        stateColor = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
                        break;
                    case MigrationState::ARRIVING:
                        stateStr = "Arriving";
                        stateColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
                        break;
                    default:
                        stateStr = "Unknown";
                        break;
                }

                ImGui::SameLine();
                ImGui::TextColored(stateColor, "[%s]", stateStr);

                // Migration type
                const char* typeStr = "";
                switch (event.type) {
                    case MigrationType::COASTAL_DRIFT: typeStr = "Coastal Drift"; break;
                    case MigrationType::FLYING: typeStr = "Flying"; break;
                    case MigrationType::FLOATING_DEBRIS: typeStr = "Rafting"; break;
                    case MigrationType::SEASONAL: typeStr = "Seasonal"; break;
                    case MigrationType::POPULATION_PRESSURE: typeStr = "Overcrowding"; break;
                    case MigrationType::FOOD_SCARCITY: typeStr = "Food Scarcity"; break;
                    case MigrationType::MATE_SEEKING: typeStr = "Mate Seeking"; break;
                    default: typeStr = "Random"; break;
                }
                ImGui::Text("Type: %s", typeStr);

                // Progress
                char progressBuf[32];
                snprintf(progressBuf, sizeof(progressBuf), "%.0f%% (%.1fs)",
                        event.progress * 100.0f, event.timeElapsed);
                ImGui::ProgressBar(event.progress, ImVec2(-1, 0), progressBuf);

                // Survival chance and energy
                ImGui::Text("Survival: %.0f%%  Energy: %.0f / %.0f",
                            event.survivalChance * 100.0f, event.currentEnergy, event.startEnergy);

                // Energy bar
                float energyRatio = event.currentEnergy / std::max(1.0f, event.startEnergy);
                ImVec4 energyColor = energyRatio > 0.5f ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) :
                                    energyRatio > 0.25f ? ImVec4(0.9f, 0.7f, 0.3f, 1.0f) :
                                    ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, energyColor);
                ImGui::ProgressBar(energyRatio, ImVec2(-1, 0), "");
                ImGui::PopStyleColor();

                ImGui::Separator();
            }
        }

        ImGui::EndChild();
    }

    ImGui::Separator();

    // Recent events from MultiIslandManager
    const auto& islandEvents = islands.getRecentEvents();
    if (!islandEvents.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Recent Events");
        ImGui::BeginChild("RecentEvents", ImVec2(0, 150), true);

        // Show last 10 events
        size_t startIdx = islandEvents.size() > 10 ? islandEvents.size() - 10 : 0;
        for (size_t i = startIdx; i < islandEvents.size(); ++i) {
            const auto& evt = islandEvents[i];
            ImVec4 eventColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);

            switch (evt.type) {
                case IslandEvent::Type::CREATURE_MIGRATED_IN:
                    eventColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
                    break;
                case IslandEvent::Type::CREATURE_MIGRATED_OUT:
                    eventColor = ImVec4(0.9f, 0.7f, 0.3f, 1.0f);
                    break;
                case IslandEvent::Type::POPULATION_BOOM:
                    eventColor = ImVec4(0.3f, 0.9f, 0.9f, 1.0f);
                    break;
                case IslandEvent::Type::POPULATION_CRASH:
                    eventColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                    break;
                default:
                    break;
            }

            ImGui::TextColored(eventColor, "[%.1fs] %s", evt.timestamp, evt.description.c_str());
        }

        ImGui::EndChild();
    }
}

// ============================================================================
// Data Recording
// ============================================================================

void IslandComparisonUI::recordHistory(const MultiIslandManager& islands) {
    // Ensure we have enough history slots
    while (m_populationHistory.size() < islands.getIslandCount()) {
        m_populationHistory.push_back(PopulationChartData());
    }

    // Record current state
    for (uint32_t i = 0; i < islands.getIslandCount(); ++i) {
        const auto* island = islands.getIsland(i);
        if (!island) continue;

        auto& history = m_populationHistory[i];
        history.timestamps.push_back(m_accumulatedTime);
        history.populations.push_back(island->stats.totalCreatures);

        // Update max
        if (island->stats.totalCreatures > history.maxPopulation) {
            history.maxPopulation = island->stats.totalCreatures;
        }

        // Trim old data
        while (history.timestamps.size() > MAX_HISTORY) {
            history.timestamps.erase(history.timestamps.begin());
            history.populations.erase(history.populations.begin());
        }
    }

    m_accumulatedTime += m_config.updateInterval;
}

// ============================================================================
// Utility Methods
// ============================================================================

std::vector<GeneFlowEdge> IslandComparisonUI::collectGeneFlowData(const MultiIslandManager& islands,
                                                                   const InterIslandMigration* migration) const {
    std::vector<GeneFlowEdge> edges;

    if (!migration) return edges;

    const auto& stats = migration->getStats();

    for (const auto& [pair, count] : stats.migrationsBetweenIslands) {
        GeneFlowEdge edge;
        edge.fromIsland = pair.first;
        edge.toIsland = pair.second;
        edge.migrationCount = count;

        // Calculate flow strength relative to max
        int maxMigrations = 1;
        for (const auto& [p, c] : stats.migrationsBetweenIslands) {
            maxMigrations = std::max(maxMigrations, c);
        }
        edge.flowStrength = static_cast<float>(count) / maxMigrations;

        // Color based on flow strength
        edge.color = glm::vec3(
            1.0f - edge.flowStrength * 0.7f,
            0.3f + edge.flowStrength * 0.6f,
            0.3f
        );

        edges.push_back(edge);
    }

    return edges;
}

glm::vec4 IslandComparisonUI::getColorForStat(float value, float min, float max) const {
    float normalized = (value - min) / (max - min);
    normalized = std::max(0.0f, std::min(1.0f, normalized));

    // Green (good) to Yellow to Red (bad) gradient
    if (normalized < 0.5f) {
        return glm::vec4(normalized * 2.0f, 0.9f, 0.3f, 1.0f);
    } else {
        return glm::vec4(0.9f, 1.0f - (normalized - 0.5f) * 2.0f * 0.6f, 0.3f, 1.0f);
    }
}

glm::vec4 IslandComparisonUI::getIslandColor(uint32_t index) const {
    if (index < m_config.islandColors.size()) {
        return m_config.islandColors[index];
    }
    return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
}

std::string IslandComparisonUI::formatNumber(int number) const {
    if (number >= 1000000) {
        return std::to_string(number / 1000000) + "M";
    } else if (number >= 1000) {
        return std::to_string(number / 1000) + "K";
    }
    return std::to_string(number);
}

std::string IslandComparisonUI::formatPercent(float value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << (value * 100.0f) << "%";
    return oss.str();
}

} // namespace Forge
