# Ecosystem Dashboard Documentation

## Table of Contents

1. [Overview](#overview)
2. [Data Collection System](#data-collection-system)
3. [Population Graphs](#population-graphs)
4. [Genetic Diversity Panel](#genetic-diversity-panel)
5. [Ecosystem Health Panel](#ecosystem-health-panel)
6. [Creature Inspector](#creature-inspector)
7. [Heatmap Visualization](#heatmap-visualization)
8. [Performance Panel](#performance-panel)
9. [Controls](#controls)
10. [Performance Impact](#performance-impact)
11. [Extending the Dashboard](#extending-the-dashboard)
12. [Technical Details](#technical-details)

---

## Overview

### Purpose of the Dashboard

The Ecosystem Dashboard provides a comprehensive real-time monitoring interface for the OrganismEvolution simulation. It enables users to observe, analyze, and understand the complex dynamics of the simulated ecosystem through visual graphs, statistics, and detailed creature inspection capabilities.

The dashboard serves several key purposes:

- **Population Monitoring**: Track herbivore, carnivore, and food populations over time
- **Genetic Analysis**: Monitor genetic diversity and trait distributions across the population
- **Ecosystem Health Assessment**: Evaluate the overall balance and sustainability of the ecosystem
- **Individual Inspection**: Examine specific creatures in detail, including their genome, energy, and behavior
- **Spatial Analysis**: Visualize population density and food distribution through heatmaps
- **Performance Monitoring**: Track simulation performance metrics

### What Metrics Are Tracked

The dashboard tracks a comprehensive set of metrics organized into categories:

| Category | Metrics |
|----------|---------|
| **Population** | Herbivore count, Carnivore count, Total creatures, Food count |
| **Genetics** | Size (avg/std), Speed (avg/std), Vision (avg/std), Efficiency (avg/std), Diversity score |
| **Health** | Predator/prey ratio, Birth rate, Death rate, Food availability, Generation stats |
| **Spatial** | 20x20 population heatmap, 20x20 food distribution heatmap |
| **Performance** | FPS, Update time, Entity counts |
| **Individual** | Energy, Health, Position, Velocity, Genome traits, Fitness, Kill count |

### ImGui-Based Real-Time Visualization

The dashboard is built using Dear ImGui (Immediate Mode GUI), a portable, renderer-agnostic GUI library. Key characteristics:

- **Immediate Mode Rendering**: UI is rebuilt every frame, enabling real-time updates
- **OpenGL Integration**: Seamlessly integrates with the GLFW/OpenGL rendering pipeline
- **Dockable Windows**: Each panel can be moved, resized, and shown/hidden independently
- **Low Overhead**: Minimal performance impact when enabled
- **Customizable Style**: Dark theme with rounded corners for readability

---

## Data Collection System

### DashboardMetrics Class

The `DashboardMetrics` class (`src/ui/DashboardMetrics.h`) is the central data collection and processing component. It aggregates simulation data each frame and maintains historical records for graphing.

**Class Structure:**

```cpp
class DashboardMetrics {
public:
    // Update metrics from simulation state
    void update(const std::vector<Creature*>& creatures,
                const std::vector<Food*>& food,
                float deltaTime);

    // Population history (for graphs)
    std::vector<float> herbivoreHistory;
    std::vector<float> carnivoreHistory;
    std::vector<float> totalPopHistory;
    std::vector<float> foodHistory;

    // Current population counts
    int herbivoreCount = 0;
    int carnivoreCount = 0;
    int totalCreatures = 0;
    int foodCount = 0;

    // Genetic diversity metrics
    float avgSize, stdSize;
    float avgSpeed, stdSpeed;
    float avgVision, stdVision;
    float avgEfficiency, stdEfficiency;
    float geneticDiversity;  // Combined metric 0-1

    // Ecosystem health indicators
    float predatorPreyRatio;
    float foodAvailabilityRatio;
    float ecosystemHealth;  // 0-100 health score
    int birthsThisMinute, deathsThisMinute;

    // Selected creature for inspector
    Creature* selectedCreature;
    CreatureInspectorData inspectorData;

    // Heat map data (20x20 grid)
    static constexpr int HEATMAP_SIZE = 20;
    float populationHeatmap[HEATMAP_SIZE][HEATMAP_SIZE];
    float foodHeatmap[HEATMAP_SIZE][HEATMAP_SIZE];
};
```

### How Metrics Are Computed Each Frame

The `update()` method is called once per frame with the current simulation state:

```cpp
void DashboardMetrics::update(const std::vector<Creature*>& creatures,
                               const std::vector<Food*>& food,
                               float deltaTime) {
    // 1. Update frame counter and timers
    frameCounter++;
    minuteTimer += deltaTime;
    historyTimer += deltaTime;

    // 2. Calculate FPS (exponential moving average)
    if (deltaTime > 0.0f) {
        float instantFps = 1.0f / deltaTime;
        fps = fps * 0.95f + instantFps * 0.05f;  // Smooth FPS
    }

    // 3. Update all metric categories
    updatePopulationCounts(creatures);
    updateGeneticDiversity(creatures);
    updateEcosystemHealth(creatures, food);
    updateHeatmaps(creatures, food);

    // 4. Update inspector if creature is selected
    if (selectedCreature != nullptr) {
        updateInspectorData();
    }

    // 5. Record history at 1-second intervals
    if (historyTimer >= HISTORY_INTERVAL) {
        recordHistory();
        historyTimer = 0.0f;
    }

    // 6. Reset birth/death counters every minute
    if (minuteTimer >= 60.0f) {
        birthsThisMinute = pendingBirths;
        deathsThisMinute = pendingDeaths;
        pendingBirths = 0;
        pendingDeaths = 0;
        minuteTimer = 0.0f;
    }
}
```

### History Tracking (300 Samples for Graphs)

The dashboard maintains rolling history buffers for population graphs:

```cpp
static constexpr int DASHBOARD_HISTORY_SIZE = 300;
static constexpr float HISTORY_INTERVAL = 1.0f;  // 1 second
```

**Key characteristics:**

- **300 samples** = approximately 5 minutes of data at 1 sample/second
- History vectors are pre-allocated with `reserve()` to avoid reallocations
- Oldest data is trimmed when buffer exceeds 300 entries
- Data recorded: herbivore count, carnivore count, total population, food count

**Recording and Trimming:**

```cpp
void DashboardMetrics::recordHistory() {
    herbivoreHistory.push_back(static_cast<float>(herbivoreCount));
    carnivoreHistory.push_back(static_cast<float>(carnivoreCount));
    totalPopHistory.push_back(static_cast<float>(totalCreatures));
    foodHistory.push_back(static_cast<float>(foodCount));

    trimHistory();
}

void DashboardMetrics::trimHistory() {
    while (herbivoreHistory.size() > DASHBOARD_HISTORY_SIZE) {
        herbivoreHistory.erase(herbivoreHistory.begin());
    }
    // ... repeat for other vectors
}
```

### Heatmap Generation (20x20 Grid)

The heatmap provides spatial distribution visualization using a 20x20 grid covering the 300x300 world space:

```cpp
static constexpr int HEATMAP_SIZE = 20;
static constexpr float WORLD_SIZE = 300.0f;
float cellSize = WORLD_SIZE / HEATMAP_SIZE;  // 15 units per cell
```

**Generation Process:**

1. Reset all heatmap cells to zero
2. For each alive creature/food item:
   - Convert world position (-150 to 150) to grid coordinates (0 to 19)
   - Increment the corresponding cell
3. Normalize values to 0-1 range based on maximum density

```cpp
// Convert world position to grid cell
float normalizedX = pos.x + halfWorld;  // -150..150 -> 0..300
float normalizedZ = pos.z + halfWorld;
int cellX = static_cast<int>(normalizedX / cellSize);
int cellZ = static_cast<int>(normalizedZ / cellSize);
cellX = std::clamp(cellX, 0, HEATMAP_SIZE - 1);
cellZ = std::clamp(cellZ, 0, HEATMAP_SIZE - 1);
populationHeatmap[cellX][cellZ] += 1.0f;
```

---

## Population Graphs

### Herbivore History Tracking

Herbivore population is tracked as a continuous time series and displayed as a green line graph:

```cpp
if (!metrics.herbivoreHistory.empty()) {
    ImGui::PushStyleColor(ImGuiCol_PlotLines, COLOR_GREEN);
    ImGui::PlotLines("##Herbivores", metrics.herbivoreHistory.data(),
                    static_cast<int>(metrics.herbivoreHistory.size()),
                    0, "Herbivores", 0.0f, maxHerb * 1.2f, ImVec2(-1, 60));
    ImGui::PopStyleColor();
}
```

**Visual characteristics:**

- **Color**: Green (0.2, 0.8, 0.2)
- **Height**: 60 pixels
- **Scale**: Auto-scaled with 20% headroom above maximum value
- **Label**: "Herbivores"

### Carnivore History Tracking

Carnivore population displayed as a red line graph:

```cpp
ImGui::PushStyleColor(ImGuiCol_PlotLines, COLOR_RED);
ImGui::PlotLines("##Carnivores", metrics.carnivoreHistory.data(),
                static_cast<int>(metrics.carnivoreHistory.size()),
                0, "Carnivores", 0.0f, maxCarn * 1.2f, ImVec2(-1, 60));
ImGui::PopStyleColor();
```

**Visual characteristics:**

- **Color**: Red (0.9, 0.2, 0.2)
- **Height**: 60 pixels
- **Scale**: Auto-scaled with 20% headroom

### Food Availability Over Time

Food count displayed as a yellow line graph:

```cpp
ImGui::PushStyleColor(ImGuiCol_PlotLines, COLOR_YELLOW);
ImGui::PlotLines("##Food", metrics.foodHistory.data(),
                static_cast<int>(metrics.foodHistory.size()),
                0, "Food", 0.0f, maxFood * 1.2f, ImVec2(-1, 60));
ImGui::PopStyleColor();
```

**Visual characteristics:**

- **Color**: Yellow (0.9, 0.9, 0.2)
- **Height**: 60 pixels

### Graph Rendering with ImGui::PlotLines

The `ImGui::PlotLines` function signature used:

```cpp
void PlotLines(
    const char* label,           // Unique ID (## prefix hides label)
    const float* values,         // Data array
    int values_count,            // Number of values
    int values_offset,           // Start index (0 = beginning)
    const char* overlay_text,    // Text overlay on graph
    float scale_min,             // Y-axis minimum (0.0)
    float scale_max,             // Y-axis maximum
    ImVec2 graph_size           // Width (-1 = fill), Height
);
```

**Legend Display:**

```cpp
// Color-coded legend at bottom of panel
ImGui::TextColored(COLOR_GREEN, "---");
ImGui::SameLine();
ImGui::Text("Herbivores");
ImGui::SameLine(120);
ImGui::TextColored(COLOR_RED, "---");
ImGui::SameLine();
ImGui::Text("Carnivores");
ImGui::SameLine(240);
ImGui::TextColored(COLOR_YELLOW, "---");
ImGui::SameLine();
ImGui::Text("Food");
```

---

## Genetic Diversity Panel

### Mean/Standard Deviation Calculations

The genetic diversity panel calculates statistical measures for four key traits:

| Trait | Range | Description |
|-------|-------|-------------|
| **Size** | 0.5 - 2.0 | Physical size affecting speed and energy |
| **Speed** | 5.0 - 20.0 | Movement speed |
| **Vision** | 10.0 - 50.0 | Detection distance |
| **Efficiency** | 0.5 - 1.5 | Energy consumption multiplier |

**Calculation Process:**

```cpp
void DashboardMetrics::updateGeneticDiversity(const std::vector<Creature*>& creatures) {
    // 1. Collect values from alive creatures
    std::vector<float> sizes, speeds, visions, efficiencies;
    for (const Creature* creature : creatures) {
        if (creature == nullptr || !creature->isAlive()) continue;

        const Genome& genome = creature->getGenome();
        sizes.push_back(genome.size);
        speeds.push_back(genome.speed);
        visions.push_back(genome.visionRange);
        efficiencies.push_back(genome.efficiency);
    }

    // 2. Calculate means
    avgSize = sumSize / n;
    avgSpeed = sumSpeed / n;
    avgVision = sumVision / n;
    avgEfficiency = sumEfficiency / n;

    // 3. Calculate sample standard deviations
    for (int i = 0; i < n; ++i) {
        float diffSize = sizes[i] - avgSize;
        varSize += diffSize * diffSize;
        // ... repeat for other traits
    }

    if (n > 1) {
        varSize /= (n - 1);  // Bessel's correction
    }

    stdSize = std::sqrt(varSize);
    // ... repeat for other traits
}
```

**Display Format:**

The panel uses an ImGui table with three columns:

```cpp
if (ImGui::BeginTable("GeneticTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
    ImGui::TableSetupColumn("Trait", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn("Std Dev", ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableHeadersRow();

    // Example row:
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Size");
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f", metrics.avgSize);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.3f", metrics.stdSize);

    ImGui::EndTable();
}
```

### Overall Diversity Score (0-1)

The combined genetic diversity score uses the **Coefficient of Variation (CV)** for each trait:

```cpp
// Coefficient of Variation = Std Dev / Mean
float normSizeDiv = (avgSize > 0.0f) ? stdSize / avgSize : 0.0f;
float normSpeedDiv = (avgSpeed > 0.0f) ? stdSpeed / avgSpeed : 0.0f;
float normVisionDiv = (avgVision > 0.0f) ? stdVision / avgVision : 0.0f;
float normEffDiv = (avgEfficiency > 0.0f) ? stdEfficiency / avgEfficiency : 0.0f;

// Combined score (sum of CVs, scaled to 0-1)
geneticDiversity = std::min(1.0f,
    (normSizeDiv + normSpeedDiv + normVisionDiv + normEffDiv) / 2.0f);
```

**Interpretation:**

- **0.0**: No genetic variation (monoculture)
- **0.1-0.3**: Low diversity
- **0.3-0.5**: Moderate diversity
- **0.5-0.7**: Good diversity
- **0.7-1.0**: High diversity

### Low Diversity Warning Threshold (< 0.3)

When genetic diversity falls below 0.3, a warning is displayed:

```cpp
float diversityScore = metrics.geneticDiversity;
ImVec4 divColor = COLOR_GREEN;

if (diversityScore < 0.3f) {
    divColor = COLOR_RED;
} else if (diversityScore < 0.5f) {
    divColor = COLOR_ORANGE;
} else if (diversityScore < 0.7f) {
    divColor = COLOR_YELLOW;
}

ImGui::Text("Diversity Score: ");
ImGui::SameLine();
ImGui::TextColored(divColor, "%.1f%%", diversityScore * 100.0f);

// Warning message for low diversity
if (diversityScore < 0.3f) {
    ImGui::TextColored(COLOR_RED, "WARNING: Low genetic diversity!");
    ImGui::TextColored(COLOR_ORANGE, "Population may be vulnerable.");
}
```

**Implications of low diversity:**

- Reduced adaptability to environmental changes
- Increased vulnerability to extinction events
- Potential inbreeding depression effects
- Convergent evolution limiting trait exploration

---

## Ecosystem Health Panel

### Predator/Prey Ratio Analysis

The predator/prey ratio measures the balance between carnivores and herbivores:

```cpp
if (herbivoreCount > 0) {
    predatorPreyRatio = static_cast<float>(carnivoreCount) /
                        static_cast<float>(herbivoreCount);
} else {
    predatorPreyRatio = (carnivoreCount > 0) ? 1.0f : 0.0f;
}
```

#### Ideal Range: 0.2-0.3

The ecosystem is considered balanced when carnivores comprise 20-30% of the herbivore population:

| Ratio | Status | Color | Description |
|-------|--------|-------|-------------|
| < 0.05 | Too Few Predators | Orange | Herbivores may overpopulate |
| 0.05-0.10 | Low Predation | Yellow | Slightly unbalanced |
| 0.10-0.30 | Balanced | Green | Optimal predator-prey dynamics |
| 0.30-0.50 | High Predation | Yellow | Pressure on herbivore population |
| > 0.50 | Too Many Predators | Red | Ecosystem collapse risk |

#### Color-Coded Status Indicators

```cpp
float ratio = metrics.predatorPreyRatio;
const char* ratioStatus = "Balanced";
ImVec4 ratioColor = COLOR_GREEN;

if (ratio < 0.05f) {
    ratioStatus = "Too Few Predators";
    ratioColor = COLOR_ORANGE;
} else if (ratio > 0.5f) {
    ratioStatus = "Too Many Predators";
    ratioColor = COLOR_RED;
} else if (ratio < 0.1f) {
    ratioStatus = "Low Predation";
    ratioColor = COLOR_YELLOW;
} else if (ratio > 0.3f) {
    ratioStatus = "High Predation";
    ratioColor = COLOR_YELLOW;
}

ImGui::Text("Predator/Prey Ratio:");
ImGui::ProgressBar(std::min(ratio, 1.0f), ImVec2(-1, 0));
ImGui::TextColored(ratioColor, "%.1f%% - %s", ratio * 100.0f, ratioStatus);
```

### Food Availability Ratio

Measures food availability relative to herbivore population:

```cpp
if (herbivoreCount > 0) {
    foodAvailabilityRatio = static_cast<float>(foodCount) /
                            static_cast<float>(herbivoreCount);
} else {
    foodAvailabilityRatio = (foodCount > 0) ? 10.0f : 0.0f;
}
```

- **Ideal ratio**: 1.0-2.0 food items per herbivore
- **Low ratio**: Food scarcity, herbivore population may decline
- **High ratio**: Abundant food, herbivore population may grow

### Birth/Death Rates Per Minute

Events are tracked via callback methods and summarized per minute:

```cpp
// Called externally when births/deaths occur
void DashboardMetrics::recordBirth() {
    birthsThisMinute++;
    pendingBirths++;
}

void DashboardMetrics::recordDeath() {
    deathsThisMinute++;
    pendingDeaths++;
}

// Every 60 seconds, finalize the counts
if (minuteTimer >= 60.0f) {
    birthsThisMinute = pendingBirths;
    deathsThisMinute = pendingDeaths;
    pendingBirths = 0;
    pendingDeaths = 0;
    minuteTimer = 0.0f;
}
```

**Display:**

```cpp
ImGui::TextColored(COLOR_GREEN, "Births/min: %d", metrics.birthsThisMinute);
ImGui::TextColored(COLOR_RED, "Deaths/min: %d", metrics.deathsThisMinute);
```

### Generation Tracking (Max and Average)

```cpp
// Find max generation
maxGeneration = 0;
for (int gen : generations) {
    if (gen > maxGeneration) {
        maxGeneration = gen;
    }
}

// Calculate average generation
avgGeneration = static_cast<float>(sumGen) / n;
```

**Display:**

```cpp
ImGui::Text("Max Generation: %d", metrics.maxGeneration);
ImGui::Text("Avg Generation: %.1f", metrics.avgGeneration);
```

### Overall Health Score (0-100)

The ecosystem health score combines four factors, each contributing 25 points:

```cpp
float healthScore = 0.0f;

// 1. Population Balance (25 points)
// Ideal predator-prey ratio is 0.25
float idealRatioCenter = 0.25f;
float ratioDiff = std::abs(predatorPreyRatio - idealRatioCenter);
float ratioScore = std::max(0.0f, 25.0f - ratioDiff * 50.0f);
healthScore += ratioScore;

// 2. Genetic Diversity (25 points)
// Higher diversity = better (0.1-0.5 is healthy)
float diversityScore = std::min(25.0f, geneticDiversity * 50.0f);
healthScore += diversityScore;

// 3. Food Availability (25 points)
// Ideal: 1.5 food per herbivore
float idealFoodRatio = 1.5f;
float foodDiff = std::abs(foodAvailabilityRatio - idealFoodRatio);
float foodScore = std::max(0.0f, 25.0f - foodDiff * 10.0f);
healthScore += foodScore;

// 4. Population Stability (25 points)
// Births should roughly equal deaths
int birthDeathDiff = std::abs(birthsThisMinute - deathsThisMinute);
int totalEvents = birthsThisMinute + deathsThisMinute;
float stabilityScore = 25.0f;
if (totalEvents > 0) {
    float imbalance = static_cast<float>(birthDeathDiff) /
                      static_cast<float>(totalEvents);
    stabilityScore = std::max(0.0f, 25.0f * (1.0f - imbalance));
}
healthScore += stabilityScore;

// Penalties for extreme population sizes
if (totalCreatures < 10) {
    healthScore *= 0.5f;  // Low population penalty
} else if (totalCreatures > 500) {
    healthScore *= 0.8f;  // Overpopulation penalty
}

ecosystemHealth = std::clamp(healthScore, 0.0f, 100.0f);
```

**Health Score Color Coding:**

| Score | Color | Status |
|-------|-------|--------|
| 0-30 | Red | Critical |
| 30-50 | Orange | Poor |
| 50-70 | Yellow | Fair |
| 70-100 | Green | Healthy |

---

## Creature Inspector

### How to Select Creatures (I Key + Mouse)

The Creature Inspector allows detailed examination of individual creatures. Selection is performed by:

1. **Ensure mouse is not captured** (press TAB if cursor is hidden)
2. **Position cursor over target creature**
3. **Press the I key** to select

```cpp
// I key - Select creature under cursor (ray casting)
static bool iKeyPressed = false;
if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !iKeyPressed) {
    if (dashboardMetrics && simulation && !mouseCaptured) {
        // Perform ray casting selection
        // ...
    }
    iKeyPressed = true;
}
```

### Ray Casting Implementation

The selection uses ray casting from camera through the cursor position:

```cpp
// Get mouse position
double mouseX, mouseY;
glfwGetCursorPos(window, &mouseX, &mouseY);

// Convert to normalized device coordinates (-1 to 1)
float ndcX = (2.0f * mouseX) / WINDOW_WIDTH - 1.0f;
float ndcY = 1.0f - (2.0f * mouseY) / WINDOW_HEIGHT;

// Create view-projection matrix
glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
                                        0.1f, 1000.0f);
glm::mat4 view = camera.getViewMatrix();

// Convert screen position to world ray
glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
glm::vec4 rayEye = glm::inverse(projection) * rayClip;
rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

// Find nearest creature along ray using sphere intersection
Creature* nearest = nullptr;
float nearestDist = 1000.0f;

for (auto* c : creatures) {
    if (!c || !c->isAlive()) continue;

    // Sphere intersection test
    glm::vec3 toCreature = c->getPosition() - camera.Position;
    float proj = glm::dot(toCreature, rayWorld);
    if (proj < 0) continue;  // Behind camera

    glm::vec3 closest = camera.Position + rayWorld * proj;
    float dist = glm::length(closest - c->getPosition());
    float hitRadius = c->getGenome().size * 3.0f;  // Generous hit radius

    if (dist < hitRadius && proj < nearestDist) {
        nearestDist = proj;
        nearest = c;
    }
}

if (nearest) {
    dashboardMetrics->selectCreature(nearest);
}
```

### Displayed Information

The Inspector panel displays comprehensive creature data organized into sections:

#### Identity Section
- **ID**: Unique creature identifier
- **Type**: "Herbivore" (green) or "Carnivore" (red)
- **Generation**: Evolutionary generation number
- **Status**: [DEAD] indicator if creature has died

#### Health and Energy
```cpp
// Health bar (red)
float maxHealth = 100.0f;
ImGui::Text("Health:");
ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_RED);
ImGui::ProgressBar(sel.health / maxHealth, ImVec2(-1, 0), healthOverlay);
ImGui::PopStyleColor();

// Energy bar (yellow)
float maxEnergy = 200.0f;
ImGui::Text("Energy:");
ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_YELLOW);
ImGui::ProgressBar(sel.energy / maxEnergy, ImVec2(-1, 0), energyOverlay);
ImGui::PopStyleColor();
```

#### Genome Stats
```cpp
ImGui::Text("Genome:");
ImGui::Indent();
ImGui::Text("Size: %.2f", sel.size);
ImGui::Text("Speed: %.2f", sel.speed);
ImGui::Text("Vision: %.2f", sel.visionRange);
ImGui::Unindent();
```

#### Color Preview
```cpp
ImGui::Text("Color:");
ImGui::SameLine();
ImGui::ColorButton("##CreatureColor",
                  ImVec4(sel.color.r, sel.color.g, sel.color.b, 1.0f),
                  ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                  ImVec2(30, 20));
```

#### Position and Velocity
```cpp
ImGui::Text("Position: (%.1f, %.1f, %.1f)",
           sel.position.x, sel.position.y, sel.position.z);
ImGui::Text("Speed: %.2f", sel.velocityMag);
```

#### Fitness Score
```cpp
ImGui::Text("Fitness Score: ");
ImGui::SameLine();
ImGui::TextColored(COLOR_CYAN, "%.2f", sel.fitness);
```

#### Fear Level (Color-Coded)
```cpp
ImGui::Text("Fear Level: ");
ImGui::SameLine();
if (sel.fear > 0.7f) {
    ImGui::TextColored(COLOR_RED, "%.0f%%", sel.fear * 100.0f);
} else if (sel.fear > 0.3f) {
    ImGui::TextColored(COLOR_YELLOW, "%.0f%%", sel.fear * 100.0f);
} else {
    ImGui::TextColored(COLOR_GREEN, "%.0f%%", sel.fear * 100.0f);
}
```

#### Kill Count (Carnivores Only)
```cpp
if (!isHerbivore) {
    ImGui::Text("Kill Count: ");
    ImGui::SameLine();
    ImGui::TextColored(COLOR_RED, "%d", sel.killCount);
}
```

### CreatureInspectorData Structure

```cpp
struct CreatureInspectorData {
    int id = -1;
    std::string typeStr;
    int generation = 0;
    float health = 0;
    float energy = 0;
    float age = 0;
    float size = 0;
    float speed = 0;
    float visionRange = 0;
    glm::vec3 color{0};
    glm::vec3 position{0};
    float velocityMag = 0;
    float fitness = 0;
    int killCount = 0;
    float fear = 0;
    bool alive = false;
};
```

---

## Heatmap Visualization

### 20x20 Grid Coverage

The heatmap divides the 300x300 world space into a 20x20 grid of cells:

```cpp
static constexpr int HEATMAP_SIZE = 20;
static constexpr float WORLD_SIZE = 300.0f;
float cellSize = WORLD_SIZE / HEATMAP_SIZE;  // 15 units per cell
```

Each cell covers 15x15 world units, providing a balance between detail and performance.

### Population Density Mapping

The population heatmap accumulates creature positions:

```cpp
for (const Creature* creature : creatures) {
    if (creature == nullptr || !creature->isAlive()) continue;

    const glm::vec3& pos = creature->getPosition();

    // Convert world position to grid cell
    float normalizedX = pos.x + halfWorld;  // -150..150 -> 0..300
    float normalizedZ = pos.z + halfWorld;

    int cellX = static_cast<int>(normalizedX / cellSize);
    int cellZ = static_cast<int>(normalizedZ / cellSize);

    cellX = std::clamp(cellX, 0, HEATMAP_SIZE - 1);
    cellZ = std::clamp(cellZ, 0, HEATMAP_SIZE - 1);

    populationHeatmap[cellX][cellZ] += 1.0f;
}
```

### Food Distribution Mapping

Similar process for food items:

```cpp
for (const Food* f : food) {
    if (f == nullptr || !f->isActive()) continue;

    const glm::vec3& pos = f->getPosition();
    // ... same conversion process ...
    foodHeatmap[cellX][cellZ] += 1.0f;
}
```

### Color Gradients

#### Population Heatmap (Blue to Red)
- **Blue** (0.2, 0.2, 1.0): Low density / Empty
- **Red** (1.0, 0.2, 0.2): High density / Crowded

```cpp
// Blue-to-red gradient for creatures
float r = normalized;
float b = 1.0f - normalized;
color = ImGui::ColorConvertFloat4ToU32(ImVec4(r, 0.2f, b, 1.0f));
```

#### Food Heatmap (Yellow Intensity)
- **Dark**: Low food density
- **Bright Yellow** (0.9, 0.9, 0.2): High food density

```cpp
// Yellow for food
color = ImGui::ColorConvertFloat4ToU32(
    ImVec4(normalized * 0.9f, normalized * 0.9f, normalized * 0.2f, 1.0f));
```

### Rendering Implementation

```cpp
void DashboardUI::renderHeatmapPanel(DashboardMetrics& metrics) {
    static bool showFood = false;
    ImGui::Checkbox("Show Food Density", &showFood);

    const int gridSize = DashboardMetrics::HEATMAP_SIZE;
    const float cellSize = 14.0f;  // Visual cell size in pixels

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            float density = showFood ?
                metrics.foodHeatmap[y][x] :
                metrics.populationHeatmap[y][x];

            float normalized = (maxDensity > 0.0f) ?
                (density / maxDensity) : 0.0f;

            // Calculate color based on mode
            ImU32 color = calculateColor(normalized, showFood);

            // Draw cell
            ImVec2 p0 = ImVec2(canvasPos.x + x * cellSize,
                               canvasPos.y + y * cellSize);
            ImVec2 p1 = ImVec2(p0.x + cellSize - 1, p0.y + cellSize - 1);
            drawList->AddRectFilled(p0, p1, color);
        }
    }

    // Reserve space for the heatmap
    ImGui::Dummy(ImVec2(gridSize * cellSize, gridSize * cellSize));
}
```

---

## Performance Panel

### FPS Monitoring

FPS is calculated using an exponential moving average for stability:

```cpp
// In DashboardMetrics::update()
if (deltaTime > 0.0f) {
    float instantFps = 1.0f / deltaTime;
    fps = fps * 0.95f + instantFps * 0.05f;  // Smooth FPS
}
```

**Display with color coding:**

```cpp
ImVec4 fpsColor = COLOR_GREEN;
if (metrics.fps < 30.0f) {
    fpsColor = COLOR_RED;
} else if (metrics.fps < 60.0f) {
    fpsColor = COLOR_YELLOW;
}

ImGui::Text("FPS: ");
ImGui::SameLine();
ImGui::TextColored(fpsColor, "%.1f", metrics.fps);
```

| FPS Range | Color | Status |
|-----------|-------|--------|
| 60+ | Green | Excellent |
| 30-60 | Yellow | Acceptable |
| < 30 | Red | Performance issues |

### Update Time Tracking

```cpp
ImGui::Text("Update Time: %.2f ms", metrics.updateTime);
ImGui::Text("Frame Time: %.2f ms", (metrics.fps > 0) ?
            (1000.0f / metrics.fps) : 0.0f);
```

### Entity Counts

```cpp
ImGui::Text("Entity Count: %d", metrics.totalCreatures + metrics.foodCount);
```

---

## Controls

### Keyboard Shortcuts

| Key | Function | Description |
|-----|----------|-------------|
| `` ` `` (backtick) | Toggle dashboard | Show/hide the entire dashboard UI |
| `I` | Inspect creature | Select creature under mouse cursor |
| `Tab` | Toggle mouse capture | Enable/disable mouse look (must disable for selection) |

### Implementation Details

**Backtick Toggle:**

```cpp
static bool backtickPressed = false;
if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !backtickPressed) {
    if (dashboardUI) {
        dashboardUI->toggle();
        std::cout << "[DASHBOARD] " << (dashboardUI->isVisible() ? "SHOWN" : "HIDDEN") << std::endl;
    }
    backtickPressed = true;
}
if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_RELEASE) {
    backtickPressed = false;
}
```

### Panel Menu for Showing/Hiding Individual Panels

The menu bar provides checkboxes for each panel:

```cpp
void DashboardUI::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Panels")) {
            ImGui::MenuItem("Population History", nullptr, &showPopulationPanel);
            ImGui::MenuItem("Genetic Diversity", nullptr, &showGeneticsPanel);
            ImGui::MenuItem("Ecosystem Health", nullptr, &showHealthPanel);
            ImGui::MenuItem("Creature Inspector", nullptr, &showInspectorPanel);
            ImGui::Separator();
            ImGui::MenuItem("Population Heatmap", nullptr, &showHeatmapPanel);
            ImGui::MenuItem("Performance", nullptr, &showPerformancePanel);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
```

**Panel Visibility Defaults:**

| Panel | Default State |
|-------|---------------|
| Population History | Shown |
| Genetic Diversity | Shown |
| Ecosystem Health | Shown |
| Creature Inspector | Shown |
| Population Heatmap | Hidden |
| Performance | Hidden |

---

## Performance Impact

### Metrics Update Overhead

The `DashboardMetrics::update()` function performs the following operations per frame:

| Operation | Complexity | Typical Cost |
|-----------|------------|--------------|
| Population counting | O(n) creatures | ~0.01ms for 200 creatures |
| Genetic diversity | O(n) creatures | ~0.02ms for 200 creatures |
| Ecosystem health | O(n) creatures + O(m) food | ~0.02ms |
| Heatmap generation | O(n+m) entities | ~0.01ms |
| Inspector update | O(1) | ~0.001ms |
| History recording | O(1) every second | ~0.001ms |

**Total typical overhead**: 0.05-0.1ms per frame (< 1% of 60 FPS budget)

### ImGui Rendering Cost

ImGui rendering costs depend on panel complexity:

| Panel | Approximate Cost |
|-------|-----------------|
| Population graphs | ~0.2ms (3 PlotLines calls) |
| Genetic diversity table | ~0.05ms |
| Health panel | ~0.1ms |
| Inspector panel | ~0.1ms |
| Heatmap | ~0.3ms (400 rectangle draws) |
| Performance panel | ~0.02ms |
| Menu bar | ~0.01ms |

**Total typical rendering**: 0.5-0.8ms per frame with all panels visible

### Recommendations for Low-End Systems

1. **Hide the heatmap panel** - Most expensive due to 400 rectangle draws
2. **Hide the performance panel** - Least useful for gameplay
3. **Toggle dashboard off during intensive periods** - Use backtick key
4. **Reduce history size** if memory constrained:
   ```cpp
   // In DashboardMetrics.h, reduce from 300 to 100
   static constexpr int DASHBOARD_HISTORY_SIZE = 100;
   ```
5. **Increase history interval** to reduce update frequency:
   ```cpp
   static constexpr float HISTORY_INTERVAL = 2.0f;  // 2 seconds instead of 1
   ```

---

## Extending the Dashboard

### How to Add New Metrics to DashboardMetrics

1. **Add data members** to `DashboardMetrics.h`:

```cpp
// In class DashboardMetrics public section:
float newMetric = 0.0f;
std::vector<float> newMetricHistory;  // If historical tracking needed
```

2. **Initialize in constructor** (`DashboardMetrics.cpp`):

```cpp
DashboardMetrics::DashboardMetrics() {
    // ... existing code ...
    newMetricHistory.reserve(DASHBOARD_HISTORY_SIZE);
}
```

3. **Calculate in update loop** (add a new update function):

```cpp
void DashboardMetrics::updateNewMetric(const std::vector<Creature*>& creatures) {
    // Calculate newMetric from creature data
    newMetric = 0.0f;
    for (const Creature* c : creatures) {
        if (c && c->isAlive()) {
            newMetric += someCalculation(c);
        }
    }
}
```

4. **Call from main update**:

```cpp
void DashboardMetrics::update(...) {
    // ... existing updates ...
    updateNewMetric(creatures);

    // Add to history recording if needed
    if (historyTimer >= HISTORY_INTERVAL) {
        newMetricHistory.push_back(newMetric);
        // ... existing history recording ...
    }
}
```

### How to Create New Panels in DashboardUI

1. **Add visibility toggle** in `DashboardUI.h`:

```cpp
private:
    bool showNewPanel = true;  // or false for hidden by default
```

2. **Add render function declaration**:

```cpp
private:
    void renderNewPanel(DashboardMetrics& metrics);
```

3. **Add to menu** in `renderMenuBar()`:

```cpp
ImGui::MenuItem("New Panel", nullptr, &showNewPanel);
```

4. **Call from render()** in `DashboardUI.cpp`:

```cpp
void DashboardUI::render(DashboardMetrics& metrics, Camera& camera) {
    // ... existing code ...
    if (showNewPanel) {
        renderNewPanel(metrics);
    }
}
```

5. **Implement the panel**:

```cpp
void DashboardUI::renderNewPanel(DashboardMetrics& metrics) {
    ImGui::SetNextWindowPos(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("New Panel", &showNewPanel)) {
        ImGui::Text("New Metric: %.2f", metrics.newMetric);

        // Add graphs, tables, or other visualizations
        if (!metrics.newMetricHistory.empty()) {
            ImGui::PlotLines("##NewMetric",
                            metrics.newMetricHistory.data(),
                            static_cast<int>(metrics.newMetricHistory.size()),
                            0, "History", 0.0f, 100.0f, ImVec2(-1, 60));
        }
    }
    ImGui::End();
}
```

### Adding New Visualization Types

**Example: Adding a Histogram**

```cpp
// Create histogram bins
float bins[10] = {0};
for (const Creature* c : creatures) {
    if (c && c->isAlive()) {
        int binIndex = std::clamp(static_cast<int>(c->getGenome().size * 5), 0, 9);
        bins[binIndex]++;
    }
}

// Render histogram
ImGui::PlotHistogram("##SizeDistribution", bins, 10,
                     0, "Size Distribution", 0.0f, 50.0f, ImVec2(-1, 80));
```

**Example: Adding a Custom Draw**

```cpp
// Get draw list for custom rendering
ImDrawList* drawList = ImGui::GetWindowDrawList();
ImVec2 canvasPos = ImGui::GetCursorScreenPos();

// Draw a circle
drawList->AddCircleFilled(
    ImVec2(canvasPos.x + 50, canvasPos.y + 50),  // Center
    20.0f,                                        // Radius
    IM_COL32(255, 100, 100, 255)                  // Color (RGBA)
);

// Reserve space
ImGui::Dummy(ImVec2(100, 100));
```

---

## Technical Details

### ImGui Integration with GLFW/OpenGL

The dashboard uses ImGui with GLFW and OpenGL 4.3 backends:

**Initialization** (in `DashboardUI::init()`):

```cpp
bool DashboardUI::init(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    // ... additional style customization ...

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    return true;
}
```

**Frame Lifecycle:**

```cpp
// Start of frame (before rendering)
void DashboardUI::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// End of frame (after all ImGui calls)
void DashboardUI::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

**Cleanup:**

```cpp
void DashboardUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
```

### Thread Safety Considerations

The current implementation is **single-threaded** and not thread-safe:

1. **DashboardMetrics** is updated from the main thread only
2. **Creature pointers** are stored directly - no reference counting
3. **Selected creature** may become invalid if creature dies

**Safe Practices:**

- Always check `selectedCreature != nullptr` before use
- Check `isAlive()` before accessing creature data
- Update metrics before rendering each frame
- Clear selection when creature dies:

```cpp
void DashboardMetrics::updateInspectorData() {
    if (selectedCreature == nullptr) {
        inspectorData = CreatureInspectorData();
        return;
    }

    // Check if creature is still alive
    if (!selectedCreature->isAlive()) {
        inspectorData.alive = false;
        // Keep other data for display but mark as dead
        return;
    }
    // ... update data ...
}
```

**Input Capture Prevention:**

```cpp
// Prevent camera/game input when mouse is over ImGui
bool DashboardUI::wantCaptureMouse() const {
    if (!initialized) return false;
    return ImGui::GetIO().WantCaptureMouse;
}

bool DashboardUI::wantCaptureKeyboard() const {
    if (!initialized) return false;
    return ImGui::GetIO().WantCaptureKeyboard;
}
```

### Memory Usage for History Buffers

Each history buffer stores `float` values:

| Buffer | Elements | Size |
|--------|----------|------|
| herbivoreHistory | 300 | 1,200 bytes |
| carnivoreHistory | 300 | 1,200 bytes |
| totalPopHistory | 300 | 1,200 bytes |
| foodHistory | 300 | 1,200 bytes |
| populationHeatmap | 400 | 1,600 bytes |
| foodHeatmap | 400 | 1,600 bytes |

**Total static memory**: ~8 KB

**Note**: History vectors are pre-allocated with `reserve()` to avoid reallocations during runtime, ensuring consistent performance.

---

## Appendix: Color Constants

```cpp
static const ImVec4 COLOR_GREEN  = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
static const ImVec4 COLOR_RED    = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
static const ImVec4 COLOR_YELLOW = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);
static const ImVec4 COLOR_ORANGE = ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
static const ImVec4 COLOR_CYAN   = ImVec4(0.2f, 0.8f, 0.9f, 1.0f);
static const ImVec4 COLOR_WHITE  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
```

---

## Appendix: File Locations

| Component | File Path |
|-----------|-----------|
| Metrics Class Header | `src/ui/DashboardMetrics.h` |
| Metrics Implementation | `src/ui/DashboardMetrics.cpp` |
| UI Class Header | `src/ui/DashboardUI.h` |
| UI Implementation | `src/ui/DashboardUI.cpp` |
| Integration (main) | `src/main.cpp` |
