#pragma once

#include "imgui.h"
#include "implot.h"
#include "../entities/Creature.h"
#include "../entities/Genome.h"
#include <vector>
#include <deque>
#include <string>
#include <map>
#include <memory>
#include <cmath>

namespace ui {

// ============================================================================
// Statistics Panel
// ============================================================================
// Comprehensive statistics and visualization panel including:
// - Population over time graphs
// - Species diversity graphs
// - Average fitness graphs
// - Trait distribution histograms
// - Food chain visualization
// - Real-time metrics display

// History data structure for graphs
struct PopulationHistory {
    static constexpr size_t MAX_HISTORY = 600;  // 10 minutes at 1 sample/sec

    std::deque<float> totalPopulation;
    std::deque<float> herbivoreCount;
    std::deque<float> carnivoreCount;
    std::deque<float> aquaticCount;
    std::deque<float> flyingCount;
    std::deque<float> foodCount;
    std::deque<float> timestamps;

    void addSample(float total, float herb, float carn, float aqua, float fly, float food, float time) {
        totalPopulation.push_back(total);
        herbivoreCount.push_back(herb);
        carnivoreCount.push_back(carn);
        aquaticCount.push_back(aqua);
        flyingCount.push_back(fly);
        foodCount.push_back(food);
        timestamps.push_back(time);

        // Trim old data
        while (totalPopulation.size() > MAX_HISTORY) {
            totalPopulation.pop_front();
            herbivoreCount.pop_front();
            carnivoreCount.pop_front();
            aquaticCount.pop_front();
            flyingCount.pop_front();
            foodCount.pop_front();
            timestamps.pop_front();
        }
    }

    void clear() {
        totalPopulation.clear();
        herbivoreCount.clear();
        carnivoreCount.clear();
        aquaticCount.clear();
        flyingCount.clear();
        foodCount.clear();
        timestamps.clear();
    }
};

struct FitnessHistory {
    static constexpr size_t MAX_HISTORY = 600;

    std::deque<float> avgFitness;
    std::deque<float> maxFitness;
    std::deque<float> minFitness;
    std::deque<float> geneticDiversity;
    std::deque<float> timestamps;

    void addSample(float avg, float max, float min, float diversity, float time) {
        avgFitness.push_back(avg);
        maxFitness.push_back(max);
        minFitness.push_back(min);
        geneticDiversity.push_back(diversity);
        timestamps.push_back(time);

        while (avgFitness.size() > MAX_HISTORY) {
            avgFitness.pop_front();
            maxFitness.pop_front();
            minFitness.pop_front();
            geneticDiversity.pop_front();
            timestamps.pop_front();
        }
    }

    void clear() {
        avgFitness.clear();
        maxFitness.clear();
        minFitness.clear();
        geneticDiversity.clear();
        timestamps.clear();
    }
};

// ============================================================================
// GENERATION-BASED EVOLUTION TRACKING
// ============================================================================
// Tracks fitness metrics by generation number for evolutionary progress analysis
struct GenerationEvolutionHistory {
    static constexpr size_t MAX_GENERATIONS = 500;

    std::deque<int> generations;
    std::deque<float> avgFitness;
    std::deque<float> maxFitness;
    std::deque<float> avgNeuralComplexity;  // Average neural weight magnitude
    std::deque<float> avgSpeed;
    std::deque<float> avgVision;
    std::deque<float> avgSize;
    std::deque<int> speciesCount;

    int lastRecordedGeneration = -1;

    void addGenerationSample(int gen, float avg, float max, float complexity,
                             float speed, float vision, float size, int species) {
        // Only record if this is a new generation
        if (gen <= lastRecordedGeneration) return;
        lastRecordedGeneration = gen;

        generations.push_back(gen);
        avgFitness.push_back(avg);
        maxFitness.push_back(max);
        avgNeuralComplexity.push_back(complexity);
        avgSpeed.push_back(speed);
        avgVision.push_back(vision);
        avgSize.push_back(size);
        speciesCount.push_back(species);

        while (generations.size() > MAX_GENERATIONS) {
            generations.pop_front();
            avgFitness.pop_front();
            maxFitness.pop_front();
            avgNeuralComplexity.pop_front();
            avgSpeed.pop_front();
            avgVision.pop_front();
            avgSize.pop_front();
            speciesCount.pop_front();
        }
    }

    void clear() {
        generations.clear();
        avgFitness.clear();
        maxFitness.clear();
        avgNeuralComplexity.clear();
        avgSpeed.clear();
        avgVision.clear();
        avgSize.clear();
        speciesCount.clear();
        lastRecordedGeneration = -1;
    }

    bool isEvolutionProgressing() const {
        if (avgFitness.size() < 10) return false;
        // Compare recent average to earlier average
        float recentAvg = 0.0f, earlierAvg = 0.0f;
        size_t count = std::min(avgFitness.size() / 2, size_t(10));
        for (size_t i = 0; i < count; i++) {
            recentAvg += avgFitness[avgFitness.size() - 1 - i];
            earlierAvg += avgFitness[i];
        }
        return recentAvg > earlierAvg;
    }
};

struct TraitStatistics {
    float mean = 0.0f;
    float stdDev = 0.0f;
    float min = 0.0f;
    float max = 0.0f;
    std::vector<float> samples;

    void calculate() {
        if (samples.empty()) {
            mean = stdDev = min = max = 0.0f;
            return;
        }

        // Calculate mean
        float sum = 0.0f;
        min = samples[0];
        max = samples[0];
        for (float v : samples) {
            sum += v;
            min = std::min(min, v);
            max = std::max(max, v);
        }
        mean = sum / samples.size();

        // Calculate std dev
        float variance = 0.0f;
        for (float v : samples) {
            float diff = v - mean;
            variance += diff * diff;
        }
        stdDev = std::sqrt(variance / samples.size());
    }
};

class StatisticsPanel {
public:
    StatisticsPanel();
    ~StatisticsPanel() = default;

    // Update statistics from simulation
    void update(const std::vector<std::unique_ptr<Creature>>& creatures,
                int foodCount,
                float simulationTime,
                float deltaTime);

    // Main render function
    void render();

    // Render individual sections
    void renderPopulationGraphs();
    void renderFitnessGraphs();
    void renderTraitDistributions();
    void renderSpeciesBreakdown();
    void renderFoodChainVisualization();
    void renderLiveMetrics();
    void renderEvolutionProgressGraphs();  // Generation-based evolution tracking

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

    // Graph settings
    bool showTotalPopulation = true;
    bool showHerbivores = true;
    bool showCarnivores = true;
    bool showAquatic = true;
    bool showFlying = true;
    bool showFood = true;

private:
    bool m_visible = true;

    // History data
    PopulationHistory m_populationHistory;
    FitnessHistory m_fitnessHistory;
    GenerationEvolutionHistory m_generationHistory;  // Track evolution by generation

    // Current statistics
    TraitStatistics m_sizeStats;
    TraitStatistics m_speedStats;
    TraitStatistics m_visionStats;
    TraitStatistics m_efficiencyStats;

    // Species tracking
    std::map<std::string, int> m_speciesCounts;
    int m_totalSpecies = 0;

    // Update timing
    float m_updateTimer = 0.0f;
    static constexpr float UPDATE_INTERVAL = 1.0f;  // Update graphs every second

    // Current live metrics
    int m_totalCreatures = 0;
    int m_herbivoreCount = 0;
    int m_carnivoreCount = 0;
    int m_aquaticCount = 0;
    int m_flyingCount = 0;
    float m_avgFitness = 0.0f;
    float m_maxFitness = 0.0f;
    float m_avgGeneration = 0.0f;
    int m_maxGeneration = 0;
    float m_avgEnergy = 0.0f;
    float m_avgAge = 0.0f;

    // Helper functions
    void updateTraitStatistics(const std::vector<std::unique_ptr<Creature>>& creatures);
    void updateSpeciesCounts(const std::vector<std::unique_ptr<Creature>>& creatures);
    std::vector<float> getPlotData(const std::deque<float>& data);
};

// ============================================================================
// Implementation
// ============================================================================

inline StatisticsPanel::StatisticsPanel() = default;

inline void StatisticsPanel::update(
    const std::vector<std::unique_ptr<Creature>>& creatures,
    int foodCount,
    float simulationTime,
    float deltaTime
) {
    m_updateTimer += deltaTime;

    // Update live metrics every frame
    m_totalCreatures = 0;
    m_herbivoreCount = 0;
    m_carnivoreCount = 0;
    m_aquaticCount = 0;
    m_flyingCount = 0;
    float totalFitness = 0.0f;
    float totalGeneration = 0.0f;
    float totalEnergy = 0.0f;
    float totalAge = 0.0f;
    m_maxFitness = 0.0f;
    m_maxGeneration = 0;

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        m_totalCreatures++;
        totalFitness += creature->getFitness();
        totalGeneration += creature->getGeneration();
        totalEnergy += creature->getEnergy();
        totalAge += creature->getAge();

        m_maxFitness = std::max(m_maxFitness, creature->getFitness());
        m_maxGeneration = std::max(m_maxGeneration, static_cast<int>(creature->getGeneration()));

        CreatureType type = creature->getType();
        if (isHerbivore(type)) m_herbivoreCount++;
        else if (isPredator(type)) m_carnivoreCount++;
        if (isAquatic(type)) m_aquaticCount++;
        if (isFlying(type)) m_flyingCount++;
    }

    if (m_totalCreatures > 0) {
        m_avgFitness = totalFitness / m_totalCreatures;
        m_avgGeneration = totalGeneration / m_totalCreatures;
        m_avgEnergy = totalEnergy / m_totalCreatures;
        m_avgAge = totalAge / m_totalCreatures;
    }

    // Update history at interval
    if (m_updateTimer >= UPDATE_INTERVAL) {
        m_updateTimer = 0.0f;

        // Population history
        m_populationHistory.addSample(
            static_cast<float>(m_totalCreatures),
            static_cast<float>(m_herbivoreCount),
            static_cast<float>(m_carnivoreCount),
            static_cast<float>(m_aquaticCount),
            static_cast<float>(m_flyingCount),
            static_cast<float>(foodCount),
            simulationTime
        );

        // Fitness history
        float minFitness = m_maxFitness;
        for (const auto& creature : creatures) {
            if (creature && creature->isAlive()) {
                minFitness = std::min(minFitness, creature->getFitness());
            }
        }

        // Calculate genetic diversity as coefficient of variation of key traits
        float diversity = 0.0f;
        if (m_totalCreatures > 0) {
            updateTraitStatistics(creatures);
            diversity = (m_sizeStats.stdDev / std::max(m_sizeStats.mean, 0.001f) +
                        m_speedStats.stdDev / std::max(m_speedStats.mean, 0.001f) +
                        m_visionStats.stdDev / std::max(m_visionStats.mean, 0.001f)) / 3.0f;
            diversity = std::min(diversity, 1.0f);
        }

        m_fitnessHistory.addSample(m_avgFitness, m_maxFitness, minFitness, diversity, simulationTime);

        // Update species counts
        updateSpeciesCounts(creatures);

        // =========================================================
        // GENERATION-BASED EVOLUTION TRACKING
        // Record fitness metrics when max generation increases
        // This tracks EVOLUTIONARY PROGRESS, not just time
        // =========================================================
        if (m_maxGeneration > m_generationHistory.lastRecordedGeneration && m_totalCreatures > 0) {
            // Calculate average neural weight magnitude (simple complexity measure)
            float avgNeuralComplexity = 0.0f;
            for (const auto& creature : creatures) {
                if (!creature || !creature->isAlive()) continue;
                const auto& weights = creature->getGenome().neuralWeights;
                float sum = 0.0f;
                for (float w : weights) {
                    sum += std::abs(w);
                }
                avgNeuralComplexity += (weights.empty() ? 0.0f : sum / weights.size());
            }
            avgNeuralComplexity /= m_totalCreatures;

            m_generationHistory.addGenerationSample(
                m_maxGeneration,
                m_avgFitness,
                m_maxFitness,
                avgNeuralComplexity,
                m_speedStats.mean,
                m_visionStats.mean,
                m_sizeStats.mean,
                m_totalSpecies
            );
        }
    }
}

inline void StatisticsPanel::updateTraitStatistics(const std::vector<std::unique_ptr<Creature>>& creatures) {
    m_sizeStats.samples.clear();
    m_speedStats.samples.clear();
    m_visionStats.samples.clear();
    m_efficiencyStats.samples.clear();

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        const Genome& g = creature->getGenome();
        m_sizeStats.samples.push_back(g.size);
        m_speedStats.samples.push_back(g.speed);
        m_visionStats.samples.push_back(g.visionRange);
        m_efficiencyStats.samples.push_back(g.efficiency);
    }

    m_sizeStats.calculate();
    m_speedStats.calculate();
    m_visionStats.calculate();
    m_efficiencyStats.calculate();
}

inline void StatisticsPanel::updateSpeciesCounts(const std::vector<std::unique_ptr<Creature>>& creatures) {
    m_speciesCounts.clear();

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        std::string typeName = getCreatureTypeName(creature->getType());
        m_speciesCounts[typeName]++;
    }

    m_totalSpecies = static_cast<int>(m_speciesCounts.size());
}

inline std::vector<float> StatisticsPanel::getPlotData(const std::deque<float>& data) {
    return std::vector<float>(data.begin(), data.end());
}

inline void StatisticsPanel::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Statistics & Graphs", &m_visible)) {
        // Live metrics at top
        if (ImGui::CollapsingHeader("Live Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderLiveMetrics();
        }

        // Population graphs
        if (ImGui::CollapsingHeader("Population Over Time", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderPopulationGraphs();
        }

        // Fitness graphs
        if (ImGui::CollapsingHeader("Fitness & Diversity")) {
            renderFitnessGraphs();
        }

        // Evolution progress (generation-based)
        if (ImGui::CollapsingHeader("Evolution Progress (by Generation)", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderEvolutionProgressGraphs();
        }

        // Trait distributions
        if (ImGui::CollapsingHeader("Trait Distributions")) {
            renderTraitDistributions();
        }

        // Species breakdown
        if (ImGui::CollapsingHeader("Species Breakdown")) {
            renderSpeciesBreakdown();
        }

        // Food chain
        if (ImGui::CollapsingHeader("Food Chain")) {
            renderFoodChainVisualization();
        }
    }
    ImGui::End();
}

inline void StatisticsPanel::renderLiveMetrics() {
    ImGui::Columns(4, "LiveMetricsColumns", false);

    // Column 1: Population
    ImGui::Text("Population");
    ImGui::Separator();
    ImGui::Text("Total: %d", m_totalCreatures);
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Herbivores: %d", m_herbivoreCount);
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Predators: %d", m_carnivoreCount);
    ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "Aquatic: %d", m_aquaticCount);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "Flying: %d", m_flyingCount);

    ImGui::NextColumn();

    // Column 2: Fitness
    ImGui::Text("Fitness");
    ImGui::Separator();
    ImGui::Text("Average: %.2f", m_avgFitness);
    ImGui::Text("Maximum: %.2f", m_maxFitness);

    ImGui::NextColumn();

    // Column 3: Generation
    ImGui::Text("Generation");
    ImGui::Separator();
    ImGui::Text("Average: %.1f", m_avgGeneration);
    ImGui::Text("Maximum: %d", m_maxGeneration);

    ImGui::NextColumn();

    // Column 4: Health
    ImGui::Text("Health");
    ImGui::Separator();
    ImGui::Text("Avg Energy: %.1f", m_avgEnergy);
    ImGui::Text("Avg Age: %.1fs", m_avgAge);

    ImGui::Columns(1);
}

inline void StatisticsPanel::renderPopulationGraphs() {
    // Graph toggles
    ImGui::Checkbox("Total", &showTotalPopulation);
    ImGui::SameLine();
    ImGui::Checkbox("Herbivores", &showHerbivores);
    ImGui::SameLine();
    ImGui::Checkbox("Predators", &showCarnivores);
    ImGui::SameLine();
    ImGui::Checkbox("Aquatic", &showAquatic);
    ImGui::SameLine();
    ImGui::Checkbox("Flying", &showFlying);
    ImGui::SameLine();
    ImGui::Checkbox("Food", &showFood);

    if (m_populationHistory.totalPopulation.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Collecting data...");
        return;
    }

    // Create plot
    if (ImPlot::BeginPlot("Population Over Time", ImVec2(-1, 250))) {
        ImPlot::SetupAxes("Time (s)", "Count");

        auto timestamps = getPlotData(m_populationHistory.timestamps);

        if (showTotalPopulation) {
            auto data = getPlotData(m_populationHistory.totalPopulation);
            ImPlot::SetNextLineStyle(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), 2.0f);
            ImPlot::PlotLine("Total", timestamps.data(), data.data(), static_cast<int>(data.size()));
        }

        if (showHerbivores) {
            auto data = getPlotData(m_populationHistory.herbivoreCount);
            ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
            ImPlot::PlotLine("Herbivores", timestamps.data(), data.data(), static_cast<int>(data.size()));
        }

        if (showCarnivores) {
            auto data = getPlotData(m_populationHistory.carnivoreCount);
            ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImPlot::PlotLine("Predators", timestamps.data(), data.data(), static_cast<int>(data.size()));
        }

        if (showAquatic) {
            auto data = getPlotData(m_populationHistory.aquaticCount);
            ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
            ImPlot::PlotLine("Aquatic", timestamps.data(), data.data(), static_cast<int>(data.size()));
        }

        if (showFlying) {
            auto data = getPlotData(m_populationHistory.flyingCount);
            ImPlot::SetNextLineStyle(ImVec4(0.7f, 0.7f, 0.3f, 1.0f));
            ImPlot::PlotLine("Flying", timestamps.data(), data.data(), static_cast<int>(data.size()));
        }

        if (showFood) {
            auto data = getPlotData(m_populationHistory.foodCount);
            ImPlot::SetNextLineStyle(ImVec4(0.5f, 0.8f, 0.2f, 0.7f));
            ImPlot::PlotLine("Food", timestamps.data(), data.data(), static_cast<int>(data.size()));
        }

        ImPlot::EndPlot();
    }
}

inline void StatisticsPanel::renderFitnessGraphs() {
    if (m_fitnessHistory.avgFitness.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Collecting data...");
        return;
    }

    // Fitness over time
    if (ImPlot::BeginPlot("Fitness Over Time", ImVec2(-1, 180))) {
        ImPlot::SetupAxes("Time (s)", "Fitness");

        auto timestamps = getPlotData(m_fitnessHistory.timestamps);

        auto avgData = getPlotData(m_fitnessHistory.avgFitness);
        ImPlot::SetNextLineStyle(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), 2.0f);
        ImPlot::PlotLine("Average", timestamps.data(), avgData.data(), static_cast<int>(avgData.size()));

        auto maxData = getPlotData(m_fitnessHistory.maxFitness);
        ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.9f, 0.2f, 0.8f));
        ImPlot::PlotLine("Maximum", timestamps.data(), maxData.data(), static_cast<int>(maxData.size()));

        auto minData = getPlotData(m_fitnessHistory.minFitness);
        ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.2f, 0.2f, 0.6f));
        ImPlot::PlotLine("Minimum", timestamps.data(), minData.data(), static_cast<int>(minData.size()));

        ImPlot::EndPlot();
    }

    // Genetic diversity over time
    if (ImPlot::BeginPlot("Genetic Diversity", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Time (s)", "Diversity");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Once);

        auto timestamps = getPlotData(m_fitnessHistory.timestamps);
        auto divData = getPlotData(m_fitnessHistory.geneticDiversity);

        ImPlot::SetNextFillStyle(ImVec4(0.7f, 0.3f, 0.9f, 0.5f));
        ImPlot::PlotShaded("Diversity", timestamps.data(), divData.data(), static_cast<int>(divData.size()));

        ImPlot::EndPlot();
    }
}

inline void StatisticsPanel::renderTraitDistributions() {
    if (m_sizeStats.samples.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No creature data available");
        return;
    }

    // Statistics summary
    ImGui::Columns(4, "TraitStats", true);

    ImGui::Text("Size");
    ImGui::Text("Mean: %.2f", m_sizeStats.mean);
    ImGui::Text("Std: %.2f", m_sizeStats.stdDev);
    ImGui::NextColumn();

    ImGui::Text("Speed");
    ImGui::Text("Mean: %.2f", m_speedStats.mean);
    ImGui::Text("Std: %.2f", m_speedStats.stdDev);
    ImGui::NextColumn();

    ImGui::Text("Vision");
    ImGui::Text("Mean: %.2f", m_visionStats.mean);
    ImGui::Text("Std: %.2f", m_visionStats.stdDev);
    ImGui::NextColumn();

    ImGui::Text("Efficiency");
    ImGui::Text("Mean: %.2f", m_efficiencyStats.mean);
    ImGui::Text("Std: %.2f", m_efficiencyStats.stdDev);

    ImGui::Columns(1);
    ImGui::Separator();

    // Histograms
    if (ImPlot::BeginPlot("Size Distribution", ImVec2(280, 120))) {
        ImPlot::SetupAxes("Size", "Count");
        ImPlot::PlotHistogram("Size", m_sizeStats.samples.data(),
            static_cast<int>(m_sizeStats.samples.size()), 15);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    if (ImPlot::BeginPlot("Speed Distribution", ImVec2(280, 120))) {
        ImPlot::SetupAxes("Speed", "Count");
        ImPlot::PlotHistogram("Speed", m_speedStats.samples.data(),
            static_cast<int>(m_speedStats.samples.size()), 15);
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Vision Distribution", ImVec2(280, 120))) {
        ImPlot::SetupAxes("Vision", "Count");
        ImPlot::PlotHistogram("Vision", m_visionStats.samples.data(),
            static_cast<int>(m_visionStats.samples.size()), 15);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    if (ImPlot::BeginPlot("Efficiency Distribution", ImVec2(280, 120))) {
        ImPlot::SetupAxes("Efficiency", "Count");
        ImPlot::PlotHistogram("Efficiency", m_efficiencyStats.samples.data(),
            static_cast<int>(m_efficiencyStats.samples.size()), 15);
        ImPlot::EndPlot();
    }
}

inline void StatisticsPanel::renderSpeciesBreakdown() {
    ImGui::Text("Active Species Types: %d", m_totalSpecies);
    ImGui::Separator();

    // Sort by count
    std::vector<std::pair<std::string, int>> sorted(m_speciesCounts.begin(), m_speciesCounts.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Pie chart data
    std::vector<const char*> labels;
    std::vector<double> values;

    for (const auto& [name, count] : sorted) {
        labels.push_back(name.c_str());
        values.push_back(static_cast<double>(count));
    }

    if (!values.empty() && ImPlot::BeginPlot("Species Distribution", ImVec2(250, 250), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::PlotPieChart(labels.data(), values.data(), static_cast<int>(values.size()),
            0.5, 0.5, 0.4, "%.0f", 90);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    // List view
    ImGui::BeginGroup();
    ImGui::Text("Counts:");
    for (const auto& [name, count] : sorted) {
        float percentage = m_totalCreatures > 0 ? (count * 100.0f / m_totalCreatures) : 0.0f;
        ImGui::BulletText("%s: %d (%.1f%%)", name.c_str(), count, percentage);
    }
    ImGui::EndGroup();
}

inline void StatisticsPanel::renderFoodChainVisualization() {
    ImGui::TextWrapped("Food chain structure (simplified):");
    ImGui::Separator();

    // Draw a simple food chain diagram
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    float boxWidth = 120.0f;
    float boxHeight = 30.0f;
    float levelSpacing = 50.0f;
    float xCenter = pos.x + ImGui::GetContentRegionAvail().x / 2;

    // Level 4: Apex Predators
    ImVec2 apexPos(xCenter - boxWidth/2, pos.y);
    drawList->AddRectFilled(apexPos, ImVec2(apexPos.x + boxWidth, apexPos.y + boxHeight),
        IM_COL32(180, 50, 50, 255), 4.0f);
    drawList->AddText(ImVec2(apexPos.x + 10, apexPos.y + 8), IM_COL32(255, 255, 255, 255), "Apex Predators");

    // Level 3: Secondary Consumers (two boxes)
    float y3 = pos.y + levelSpacing;
    ImVec2 smallPredPos(xCenter - boxWidth - 20, y3);
    ImVec2 omnivorePos(xCenter + 20, y3);

    drawList->AddRectFilled(smallPredPos, ImVec2(smallPredPos.x + boxWidth, smallPredPos.y + boxHeight),
        IM_COL32(200, 100, 50, 255), 4.0f);
    drawList->AddText(ImVec2(smallPredPos.x + 5, smallPredPos.y + 8), IM_COL32(255, 255, 255, 255), "Small Predators");

    drawList->AddRectFilled(omnivorePos, ImVec2(omnivorePos.x + boxWidth, omnivorePos.y + boxHeight),
        IM_COL32(180, 130, 50, 255), 4.0f);
    drawList->AddText(ImVec2(omnivorePos.x + 20, omnivorePos.y + 8), IM_COL32(255, 255, 255, 255), "Omnivores");

    // Level 2: Primary Consumers (Herbivores)
    float y2 = pos.y + levelSpacing * 2;
    ImVec2 herbPos(xCenter - boxWidth/2, y2);
    drawList->AddRectFilled(herbPos, ImVec2(herbPos.x + boxWidth, herbPos.y + boxHeight),
        IM_COL32(80, 180, 80, 255), 4.0f);
    drawList->AddText(ImVec2(herbPos.x + 20, herbPos.y + 8), IM_COL32(255, 255, 255, 255), "Herbivores");

    // Level 1: Producers (Plants)
    float y1 = pos.y + levelSpacing * 3;
    ImVec2 plantPos(xCenter - boxWidth/2, y1);
    drawList->AddRectFilled(plantPos, ImVec2(plantPos.x + boxWidth, plantPos.y + boxHeight),
        IM_COL32(50, 150, 50, 255), 4.0f);
    drawList->AddText(ImVec2(plantPos.x + 15, plantPos.y + 8), IM_COL32(255, 255, 255, 255), "Plants (Food)");

    // Draw arrows
    auto drawArrow = [drawList](ImVec2 from, ImVec2 to) {
        drawList->AddLine(from, to, IM_COL32(200, 200, 200, 200), 2.0f);
        // Arrowhead
        ImVec2 dir = ImVec2(to.x - from.x, to.y - from.y);
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len; dir.y /= len;
        ImVec2 perp(-dir.y, dir.x);
        drawList->AddTriangleFilled(
            to,
            ImVec2(to.x - dir.x * 8 + perp.x * 4, to.y - dir.y * 8 + perp.y * 4),
            ImVec2(to.x - dir.x * 8 - perp.x * 4, to.y - dir.y * 8 - perp.y * 4),
            IM_COL32(200, 200, 200, 200)
        );
    };

    // Arrows from herbivores to predators
    drawArrow(ImVec2(herbPos.x + boxWidth/2, herbPos.y),
              ImVec2(smallPredPos.x + boxWidth/2, smallPredPos.y + boxHeight));
    drawArrow(ImVec2(herbPos.x + boxWidth/2, herbPos.y),
              ImVec2(omnivorePos.x + boxWidth/2, omnivorePos.y + boxHeight));

    // Arrows from plants to herbivores
    drawArrow(ImVec2(plantPos.x + boxWidth/2, plantPos.y),
              ImVec2(herbPos.x + boxWidth/2, herbPos.y + boxHeight));

    // Arrows to apex
    drawArrow(ImVec2(smallPredPos.x + boxWidth/2, smallPredPos.y),
              ImVec2(apexPos.x + boxWidth/2, apexPos.y + boxHeight));
    drawArrow(ImVec2(omnivorePos.x + boxWidth/2, omnivorePos.y),
              ImVec2(apexPos.x + boxWidth/2, apexPos.y + boxHeight));

    // Reserve space for the diagram
    ImGui::Dummy(ImVec2(0, levelSpacing * 4 + boxHeight + 10));
}

// ============================================================================
// EVOLUTION PROGRESS GRAPHS (Generation-based tracking)
// ============================================================================
inline void StatisticsPanel::renderEvolutionProgressGraphs() {
    if (m_generationHistory.generations.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "Waiting for new generations...");
        ImGui::TextWrapped("Evolution progress is tracked when new generations appear. "
                          "Reproduce creatures to advance generations!");
        return;
    }

    // Evolution status indicator
    bool isProgressing = m_generationHistory.isEvolutionProgressing();
    if (isProgressing) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Evolution is PROGRESSING!");
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "Evolution stabilizing or regressing");
    }

    ImGui::Text("Generations tracked: %d to %d",
                m_generationHistory.generations.empty() ? 0 : m_generationHistory.generations.front(),
                m_generationHistory.generations.empty() ? 0 : m_generationHistory.generations.back());
    ImGui::Separator();

    // Convert deques to vectors for ImPlot
    std::vector<float> gens(m_generationHistory.generations.begin(), m_generationHistory.generations.end());
    std::vector<float> avgFit(m_generationHistory.avgFitness.begin(), m_generationHistory.avgFitness.end());
    std::vector<float> maxFit(m_generationHistory.maxFitness.begin(), m_generationHistory.maxFitness.end());
    std::vector<float> complexity(m_generationHistory.avgNeuralComplexity.begin(), m_generationHistory.avgNeuralComplexity.end());
    std::vector<float> speed(m_generationHistory.avgSpeed.begin(), m_generationHistory.avgSpeed.end());
    std::vector<float> vision(m_generationHistory.avgVision.begin(), m_generationHistory.avgVision.end());
    std::vector<float> size(m_generationHistory.avgSize.begin(), m_generationHistory.avgSize.end());

    // Fitness over generations - THE KEY EVOLUTION INDICATOR
    if (ImPlot::BeginPlot("Fitness vs Generation", ImVec2(-1, 180))) {
        ImPlot::SetupAxes("Generation", "Fitness");

        ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), 2.5f);
        ImPlot::PlotLine("Max Fitness", gens.data(), maxFit.data(), static_cast<int>(gens.size()));

        ImPlot::SetNextLineStyle(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), 2.0f);
        ImPlot::PlotLine("Avg Fitness", gens.data(), avgFit.data(), static_cast<int>(gens.size()));

        // Add a trend line if enough data
        if (gens.size() >= 5) {
            // Simple linear regression for trend
            float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
            int n = static_cast<int>(gens.size());
            for (int i = 0; i < n; i++) {
                sumX += gens[i];
                sumY += avgFit[i];
                sumXY += gens[i] * avgFit[i];
                sumX2 += gens[i] * gens[i];
            }
            float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
            float intercept = (sumY - slope * sumX) / n;

            // Draw trend line
            std::vector<float> trendX = {gens.front(), gens.back()};
            std::vector<float> trendY = {intercept + slope * gens.front(), intercept + slope * gens.back()};
            ImPlot::SetNextLineStyle(ImVec4(1.0f, 1.0f, 0.0f, 0.6f), 1.5f);
            ImPlot::PlotLine("Trend", trendX.data(), trendY.data(), 2);
        }

        ImPlot::EndPlot();
    }

    // Trait evolution over generations
    if (ImPlot::BeginPlot("Trait Evolution", ImVec2(-1, 140))) {
        ImPlot::SetupAxes("Generation", "Value");

        ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
        ImPlot::PlotLine("Avg Speed", gens.data(), speed.data(), static_cast<int>(gens.size()));

        ImPlot::SetNextLineStyle(ImVec4(0.4f, 0.9f, 0.4f, 1.0f));
        ImPlot::PlotLine("Avg Vision", gens.data(), vision.data(), static_cast<int>(gens.size()));

        ImPlot::SetNextLineStyle(ImVec4(0.4f, 0.4f, 0.9f, 1.0f));
        ImPlot::PlotLine("Avg Size", gens.data(), size.data(), static_cast<int>(gens.size()));

        ImPlot::EndPlot();
    }

    // Neural complexity evolution
    if (ImPlot::BeginPlot("Neural Complexity", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Generation", "Avg |Weight|");

        ImPlot::SetNextFillStyle(ImVec4(0.8f, 0.3f, 0.8f, 0.5f));
        ImPlot::PlotShaded("Neural Complexity", gens.data(), complexity.data(), static_cast<int>(gens.size()));

        ImPlot::EndPlot();
    }

    // Summary statistics
    ImGui::Separator();
    ImGui::Columns(3, "EvolutionSummary", false);

    ImGui::Text("First Gen Fitness:");
    if (!avgFit.empty()) {
        ImGui::Text("  Avg: %.1f", avgFit.front());
        ImGui::Text("  Max: %.1f", maxFit.front());
    }

    ImGui::NextColumn();

    ImGui::Text("Latest Gen Fitness:");
    if (!avgFit.empty()) {
        ImGui::Text("  Avg: %.1f", avgFit.back());
        ImGui::Text("  Max: %.1f", maxFit.back());
    }

    ImGui::NextColumn();

    ImGui::Text("Improvement:");
    if (avgFit.size() >= 2) {
        float avgImprovement = avgFit.back() - avgFit.front();
        float maxImprovement = maxFit.back() - maxFit.front();
        ImVec4 color = avgImprovement > 0 ? ImVec4(0.2f, 0.9f, 0.2f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(color, "  Avg: %+.1f", avgImprovement);
        color = maxImprovement > 0 ? ImVec4(0.2f, 0.9f, 0.2f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(color, "  Max: %+.1f", maxImprovement);
    }

    ImGui::Columns(1);
}

} // namespace ui
