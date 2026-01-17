#include "PopulationGraphs.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace statistics {

// ============================================================================
// Data Preparation
// ============================================================================

void PopulationGraphs::prepareData(const StatisticsDataManager& data) const {
    const auto& history = data.getPopulationHistory();

    if (history.empty()) return;

    // Clear and reserve
    m_times.clear();
    m_totals.clear();
    m_herbivores.clear();
    m_carnivores.clear();
    m_omnivores.clear();
    m_aquatic.clear();
    m_flying.clear();
    m_land.clear();

    size_t n = history.size();
    m_times.reserve(n);
    m_totals.reserve(n);
    m_herbivores.reserve(n);
    m_carnivores.reserve(n);
    m_omnivores.reserve(n);
    m_aquatic.reserve(n);
    m_flying.reserve(n);
    m_land.reserve(n);

    // Determine time window
    float latestTime = history.back().time;
    float startTime = std::max(0.0f, latestTime - m_timeWindow);

    // Find starting index
    size_t startIdx = 0;
    for (size_t i = 0; i < history.size(); ++i) {
        if (history[i].time >= startTime) {
            startIdx = i;
            break;
        }
    }

    // Extract data within time window
    for (size_t i = startIdx; i < history.size(); ++i) {
        const auto& sample = history[i];
        m_times.push_back(sample.time);
        m_totals.push_back(static_cast<float>(sample.totalCreatures));
        m_herbivores.push_back(static_cast<float>(sample.herbivoreCount));
        m_carnivores.push_back(static_cast<float>(sample.carnivoreCount));
        m_omnivores.push_back(static_cast<float>(sample.omnivoreCount));
        m_aquatic.push_back(static_cast<float>(sample.aquaticCount));
        m_flying.push_back(static_cast<float>(sample.flyingCount));

        // Land = total - aquatic - flying (avoiding double counting)
        int landCount = sample.totalCreatures - sample.aquaticCount - sample.flyingCount;
        m_land.push_back(static_cast<float>(std::max(0, landCount)));
    }
}

void PopulationGraphs::calculateRates(const std::vector<float>& values) const {
    m_rateBuffer.clear();
    if (values.size() < 2 || m_times.size() < 2) return;

    m_rateBuffer.reserve(values.size());
    m_rateBuffer.push_back(0.0f);  // No rate for first point

    for (size_t i = 1; i < values.size(); ++i) {
        float dt = m_times[i] - m_times[i-1];
        if (dt > 0.001f) {
            float rate = (values[i] - values[i-1]) / dt;
            m_rateBuffer.push_back(rate);
        } else {
            m_rateBuffer.push_back(0.0f);
        }
    }
}

// ============================================================================
// Main Render
// ============================================================================

void PopulationGraphs::render(const StatisticsDataManager& data) {
    prepareData(data);

    if (m_times.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No population data yet...");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));

    if (ImGui::CollapsingHeader("Population Over Time", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTotalPopulation(data);
    }

    if (ImGui::CollapsingHeader("Population by Role")) {
        renderPopulationByRole(data);
    }

    if (ImGui::CollapsingHeader("Population by Domain")) {
        renderPopulationByDomain(data);
    }

    if (ImGui::CollapsingHeader("Predator-Prey Dynamics")) {
        renderPredatorPreyPhase(data);
    }

    if (ImGui::CollapsingHeader("Population Rates")) {
        renderPopulationRates(data);
    }

    ImGui::PopStyleColor();
}

void PopulationGraphs::renderCompact(const StatisticsDataManager& data) {
    prepareData(data);

    if (m_times.empty()) {
        ImGui::TextDisabled("Waiting for data...");
        return;
    }

    // Current stats
    const auto& current = data.getCurrentPopulation();
    ImGui::Text("Total: %d | Species: %d", current.totalCreatures, current.speciesCount);

    // Mini sparkline-style graph
    float savedHeight = m_graphHeight;
    m_graphHeight = 80.0f;

    if (ImPlot::BeginPlot("##PopMini", ImVec2(-1, m_graphHeight),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, nullptr,
                          ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels,
                          ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);

        if (!m_times.empty()) {
            ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);
        }

        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Total);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.0f, 0.447f, 0.698f, 0.3f));
        ImPlot::PlotShaded("##Total", m_times.data(), m_totals.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PlotLine("##Total", m_times.data(), m_totals.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor(2);

        ImPlot::EndPlot();
    }

    m_graphHeight = savedHeight;
}

// ============================================================================
// Individual Graph Renderers
// ============================================================================

void PopulationGraphs::renderTotalPopulation(const StatisticsDataManager& data) {
    if (m_times.empty()) return;

    setupPlotStyle();

    if (ImPlot::BeginPlot("Total Population", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Time (s)", "Population");
        ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);

        // Total population with shaded area
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Total);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.0f, 0.447f, 0.698f, 0.3f));
        ImPlot::PlotShaded("Total", m_times.data(), m_totals.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PlotLine("Total", m_times.data(), m_totals.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor(2);

        // Show current value annotation
        if (!m_times.empty()) {
            float lastTime = m_times.back();
            float lastVal = m_totals.back();
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(lastVal));
            ImPlot::Annotation(lastTime, lastVal, Colors::Total, ImVec2(5, -10), false, "%s", buf);
        }

        ImPlot::EndPlot();
    }

    resetPlotStyle();
}

void PopulationGraphs::renderPopulationByRole(const StatisticsDataManager& data) {
    if (m_times.empty()) return;

    setupPlotStyle();

    if (ImPlot::BeginPlot("Population by Role", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Time (s)", "Population");
        ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);

        // Herbivores
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Herbivore);
        ImPlot::PlotLine("Herbivores", m_times.data(), m_herbivores.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        // Carnivores
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Carnivore);
        ImPlot::PlotLine("Carnivores", m_times.data(), m_carnivores.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        // Omnivores
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Omnivore);
        ImPlot::PlotLine("Omnivores", m_times.data(), m_omnivores.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    resetPlotStyle();

    // Show current breakdown
    const auto& current = data.getCurrentPopulation();
    int total = std::max(1, current.totalCreatures);

    ImGui::ProgressBar(static_cast<float>(current.herbivoreCount) / total,
                       ImVec2(-1, 0), nullptr);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(Colors::Herbivore), "Herbivores: %d (%.1f%%)",
                       current.herbivoreCount,
                       100.0f * current.herbivoreCount / total);

    ImGui::ProgressBar(static_cast<float>(current.carnivoreCount) / total,
                       ImVec2(-1, 0), nullptr);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(Colors::Carnivore), "Carnivores: %d (%.1f%%)",
                       current.carnivoreCount,
                       100.0f * current.carnivoreCount / total);

    ImGui::ProgressBar(static_cast<float>(current.omnivoreCount) / total,
                       ImVec2(-1, 0), nullptr);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(Colors::Omnivore), "Omnivores: %d (%.1f%%)",
                       current.omnivoreCount,
                       100.0f * current.omnivoreCount / total);
}

void PopulationGraphs::renderStackedPopulation(const StatisticsDataManager& data) {
    if (m_times.empty()) return;

    // Create stacked data
    std::vector<float> herbStack = m_herbivores;
    std::vector<float> carnStack(m_carnivores.size());
    std::vector<float> omniStack(m_omnivores.size());

    for (size_t i = 0; i < m_carnivores.size(); ++i) {
        carnStack[i] = herbStack[i] + m_carnivores[i];
    }
    for (size_t i = 0; i < m_omnivores.size(); ++i) {
        omniStack[i] = carnStack[i] + m_omnivores[i];
    }

    setupPlotStyle();

    if (ImPlot::BeginPlot("Population Composition", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Time (s)", "Population");
        ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);

        // Render from top to bottom (so overlapping looks right)
        ImPlot::PushStyleColor(ImPlotCol_Fill, Colors::Omnivore);
        ImPlot::PlotShaded("Omnivores", m_times.data(), omniStack.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PopStyleColor();

        ImPlot::PushStyleColor(ImPlotCol_Fill, Colors::Carnivore);
        ImPlot::PlotShaded("Carnivores", m_times.data(), carnStack.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PopStyleColor();

        ImPlot::PushStyleColor(ImPlotCol_Fill, Colors::Herbivore);
        ImPlot::PlotShaded("Herbivores", m_times.data(), herbStack.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    resetPlotStyle();
}

void PopulationGraphs::renderPopulationByDomain(const StatisticsDataManager& data) {
    if (m_times.empty()) return;

    setupPlotStyle();

    if (ImPlot::BeginPlot("Population by Domain", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Time (s)", "Population");
        ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast);

        // Land
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Land);
        ImPlot::PlotLine("Land", m_times.data(), m_land.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        // Water
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Water);
        ImPlot::PlotLine("Water", m_times.data(), m_aquatic.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        // Air
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Air);
        ImPlot::PlotLine("Air", m_times.data(), m_flying.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    resetPlotStyle();

    // Domain pie chart using ImGui
    const auto& current = data.getCurrentPopulation();
    int total = std::max(1, current.totalCreatures);
    int landCount = current.totalCreatures - current.aquaticCount - current.flyingCount;
    landCount = std::max(0, landCount);

    ImGui::Columns(3, nullptr, false);
    ImGui::TextColored(Colors::Land, "Land: %d", landCount);
    ImGui::NextColumn();
    ImGui::TextColored(Colors::Water, "Water: %d", current.aquaticCount);
    ImGui::NextColumn();
    ImGui::TextColored(Colors::Air, "Air: %d", current.flyingCount);
    ImGui::Columns(1);
}

void PopulationGraphs::renderSpeciesPopulations(const StatisticsDataManager& data,
                                                 const genetics::SpeciationTracker* tracker) {
    if (!tracker || m_times.empty()) return;

    const auto& history = data.getPopulationHistory();
    const auto& allSpecies = tracker->getAllSpecies();

    // Collect species to display (only active species)
    std::vector<const genetics::Species*> activeSpecies;
    for (const auto& species : allSpecies) {
        if (species.isExtant() && species.currentPopulation > 0) {
            activeSpecies.push_back(&species);
        }
    }

    if (activeSpecies.empty()) {
        ImGui::TextDisabled("No active species to display");
        return;
    }

    // Sort by population (descending)
    std::sort(activeSpecies.begin(), activeSpecies.end(),
              [](const genetics::Species* a, const genetics::Species* b) {
                  return a->currentPopulation > b->currentPopulation;
              });

    // Limit to top 10 species
    if (activeSpecies.size() > 10) {
        activeSpecies.resize(10);
    }

    setupPlotStyle();

    if (ImPlot::BeginPlot("Species Populations", ImVec2(-1, m_graphHeight + 50))) {
        ImPlot::SetupAxes("Time (s)", "Population");
        ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);

        // Plot each species
        for (const auto* species : activeSpecies) {
            // Build time series for this species
            std::vector<float> speciesData;
            speciesData.reserve(history.size());

            for (const auto& sample : history) {
                auto it = sample.speciesPopulations.find(species->id);
                if (it != sample.speciesPopulations.end()) {
                    speciesData.push_back(static_cast<float>(it->second));
                } else {
                    speciesData.push_back(0.0f);
                }
            }

            // Filter to time window
            std::vector<float> filteredData;
            float latestTime = m_times.back();
            float startTime = std::max(0.0f, latestTime - m_timeWindow);

            size_t startIdx = 0;
            for (size_t i = 0; i < history.size(); ++i) {
                if (history[i].time >= startTime) {
                    startIdx = i;
                    break;
                }
            }

            for (size_t i = startIdx; i < speciesData.size(); ++i) {
                filteredData.push_back(speciesData[i]);
            }

            // Use species display color
            ImVec4 color(species->displayColor.r, species->displayColor.g,
                        species->displayColor.b, 1.0f);
            ImPlot::PushStyleColor(ImPlotCol_Line, color);
            ImPlot::PlotLine(species->name.c_str(), m_times.data(), filteredData.data(),
                            static_cast<int>(std::min(m_times.size(), filteredData.size())));
            ImPlot::PopStyleColor();
        }

        ImPlot::EndPlot();
    }

    resetPlotStyle();
}

void PopulationGraphs::renderPopulationRates(const StatisticsDataManager& data) {
    if (m_times.size() < 2) return;

    calculateRates(m_totals);

    setupPlotStyle();

    if (ImPlot::BeginPlot("Population Growth Rate", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Time (s)", "Rate (creatures/s)");
        ImPlot::SetupAxisLimits(ImAxis_X1, m_times.front(), m_times.back(), ImPlotCond_Always);

        // Add zero line
        float zero = 0.0f;
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
        ImPlot::PlotHLines("##zero", &zero, 1);
        ImPlot::PopStyleColor();

        // Rate with positive/negative coloring
        std::vector<float> positiveRates(m_rateBuffer.size());
        std::vector<float> negativeRates(m_rateBuffer.size());

        for (size_t i = 0; i < m_rateBuffer.size(); ++i) {
            if (m_rateBuffer[i] >= 0) {
                positiveRates[i] = m_rateBuffer[i];
                negativeRates[i] = 0.0f;
            } else {
                positiveRates[i] = 0.0f;
                negativeRates[i] = m_rateBuffer[i];
            }
        }

        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.0f, 0.8f, 0.2f, 0.5f));
        ImPlot::PlotShaded("Growth", m_times.data(), positiveRates.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PopStyleColor();

        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.8f, 0.2f, 0.2f, 0.5f));
        ImPlot::PlotShaded("Decline", m_times.data(), negativeRates.data(),
                          static_cast<int>(m_times.size()), 0);
        ImPlot::PopStyleColor();

        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::Total);
        ImPlot::PlotLine("Rate", m_times.data(), m_rateBuffer.data(),
                        static_cast<int>(m_times.size()));
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }

    resetPlotStyle();
}

void PopulationGraphs::renderPredatorPreyPhase(const StatisticsDataManager& data) {
    if (m_herbivores.size() < 2 || m_carnivores.size() < 2) {
        ImGui::TextDisabled("Need more data for phase plot...");
        return;
    }

    setupPlotStyle();

    if (ImPlot::BeginPlot("Predator-Prey Phase Space", ImVec2(-1, m_graphHeight))) {
        ImPlot::SetupAxes("Herbivore Population", "Carnivore Population");

        // Plot the phase trajectory
        ImPlot::PushStyleColor(ImPlotCol_Line, Colors::ReddishPurple);
        ImPlot::GetStyle().LineWeight = 1.5f;
        ImPlot::PlotLine("##Trajectory", m_herbivores.data(), m_carnivores.data(),
                        static_cast<int>(m_herbivores.size()));
        ImPlot::PopStyleColor();

        // Mark current position
        if (!m_herbivores.empty() && !m_carnivores.empty()) {
            float currentH = m_herbivores.back();
            float currentC = m_carnivores.back();

            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, Colors::Vermillion);
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 8.0f);
            ImPlot::PlotScatter("Current", &currentH, &currentC, 1);
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
        }

        // Mark starting position
        if (m_herbivores.size() > 1) {
            float startH = m_herbivores.front();
            float startC = m_carnivores.front();

            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, Colors::BluishGreen);
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 6.0f);
            ImPlot::PlotScatter("Start", &startH, &startC, 1);
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
        }

        ImPlot::EndPlot();
    }

    resetPlotStyle();

    // Lotka-Volterra style info
    ImGui::TextWrapped("Phase plot shows predator-prey dynamics. "
                       "Closed orbits indicate stable oscillations (classic Lotka-Volterra behavior).");
}

} // namespace statistics
} // namespace ui
