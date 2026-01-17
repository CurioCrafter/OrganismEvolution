# UI Design Documentation

## Dashboard Layout

### Visual Layout
```
+------------------+------------------------+------------------+
|  Left Panel      |                        |  Right Panel     |
|  (Dashboard)     |     Main Viewport      |  (Inspector)     |
|  320px           |     (3D Scene)         |  300px           |
|                  |                        |                  |
|  [Tab: Overview] |                        |  Creature        |
|  [Tab: Genetics] |                        |  Inspector       |
|  [Tab: Neural ]  |                        |                  |
|  [Tab: World  ]  |                        |  - Stats         |
|                  |                        |  - Genome        |
|  Performance     |                        |  - Actions       |
|  Simulation      |                        |                  |
|  Population      |                        |  Creature List   |
|  Graphs          |                        |                  |
+------------------+------------------------+------------------+
|                      Status Bar (25px)                       |
|  FPS | Creatures | Gen | Time | Health | State               |
+-------------------------------------------------------------+
```

## Panel Specifications

### Left Panel - Simulation Dashboard

**Size**: 320x700px (resizable)
**Position**: Top-left corner (10, 10)

#### Tab: Overview
1. **Performance** (Collapsible, DefaultOpen)
   - FPS counter
   - Frame time display
   - Mini frame time graph (40px height)

2. **Simulation** (Collapsible, DefaultOpen)
   - Pause checkbox with Step button
   - Speed preset buttons (0.5x, 1x, 2x, 4x, 8x)
   - Speed slider (0.1x - 10.0x)

3. **Population** (Collapsible, DefaultOpen)
   - Total creatures count
   - Herbivores (green text)
   - Carnivores (red text)
   - Aquatic (blue text, if present)
   - Flying (yellow text, if present)
   - Food sources count
   - H/C ratio progress bar
   - Birth/death rates
   - Generation stats

4. **Population Graphs** (Collapsible, DefaultOpen)
   - ImPlot line chart (200px height)
   - Series: Herbivores, Carnivores, Food
   - X-axis: Time (seconds)
   - Y-axis: Count

5. **Ecosystem Health** (Collapsible)
   - Health score with color indicator
   - Health progress bar
   - Predator-prey ratio
   - Food availability ratio
   - Genetic diversity score
   - Average creature energy/age

6. **Spawn Controls** (Collapsible)
   - Spawn 10 Herbivores button
   - Spawn 5 Carnivores button
   - Spawn 20 Food button

7. **Chaos Controls** (Collapsible)
   - Warning text
   - Kill All Carnivores button
   - Kill All Herbivores button
   - Mass Extinction 50% button
   - Mass Extinction 90% button
   - Food Boom x100 button

#### Tab: Genetics
1. **Genetic Diversity** (Collapsible, DefaultOpen)
   - Size: avg, std
   - Speed: avg, std
   - Vision: avg, std
   - Efficiency: avg, std
   - Diversity score bar

2. **Trait Distributions** (Collapsible, DefaultOpen)
   - Size histogram (ImPlot, 120px)
   - Speed histogram (ImPlot, 120px)

3. **Fitness Evolution** (Collapsible, DefaultOpen)
   - Health over time graph (ImPlot, 150px)

4. **Species Breakdown** (Collapsible)
   - Species list with color indicators
   - Member count, avg fitness, generation

#### Tab: Neural
1. **Neural Network Panel**
   - Best creature identifier
   - Generation and fitness
   - Brain Inputs/Outputs section
   - Brain Statistics section

#### Tab: World
1. **Day/Night Cycle** (Collapsible, DefaultOpen)
   - Time of day string
   - Time slider (0.0 - 1.0)
   - Day length slider (30s - 600s)
   - Pause cycle checkbox
   - Quick time buttons (Dawn, Noon, Dusk, Night)
   - Sky color preview

2. **Camera** (Collapsible, DefaultOpen)
   - Position display
   - Rotation (yaw/pitch)
   - FOV display
   - Sensitivity slider
   - Move speed slider
   - Invert axis checkboxes
   - Reset camera button

3. **Graphics** (Collapsible)
   - Show nametags checkbox
   - Show trees checkbox
   - Nametag distance slider

### Right Panel - Creature Inspector

**Size**: 300x500px (resizable)
**Position**: Top-right corner (viewport.width - 310, 10)

1. **Header**
   - "Select a creature..." message when none selected

2. **Selected Creature Info**
   - ID and Type
   - Generation
   - Energy progress bar
   - Age
   - Fitness
   - Position
   - Speed

3. **Genome Section** (Collapsible, DefaultOpen)
   - Size gene
   - Speed gene
   - Vision range
   - Efficiency
   - Color preview

4. **Combat Section** (Carnivores only, Collapsible)
   - Kill count
   - Fear level

5. **Action Buttons**
   - Follow Camera
   - Deselect

6. **Creature List** (Collapsible)
   - Scrollable child window (200px)
   - Top 15 creatures by fitness
   - Format: "H #123 - Fit:45.2 Gen:5"
   - Click to select

### Status Bar

**Size**: Full viewport width x 25px
**Position**: Bottom of viewport

**Contents (left to right)**:
- FPS: 60
- Creatures: 1000
- Gen: 50
- Time: 12:34
- Health: 75% (color-coded)
- PAUSED or RUNNING (1.0x)
- Help hint (right-aligned): "F1 = Help | F3 = Toggle UI"

### Help Window

**Trigger**: F1 key
**Size**: 400x350px
**Position**: Center of viewport

**Sections**:
1. Keyboard Shortcuts
2. Camera Controls
3. Tips

## Color Scheme

### Theme Colors
```cpp
WindowBg:     (0.08, 0.08, 0.10, 0.95)
Header:       (0.22, 0.22, 0.28, 0.80)
Button:       (0.20, 0.22, 0.28, 1.00)
FrameBg:      (0.12, 0.12, 0.16, 1.00)
PlotLines:    (0.45, 0.70, 0.95, 1.00)
```

### Status Colors
| State | Color (RGB) |
|-------|-------------|
| Healthy/Good | (0.3, 0.8, 0.3) |
| Warning | (0.8, 0.8, 0.3) |
| Critical/Bad | (0.8, 0.3, 0.3) |
| Herbivore | (0.3, 0.8, 0.3) |
| Carnivore | (0.9, 0.3, 0.3) |
| Aquatic | (0.3, 0.6, 0.9) |
| Flying | (0.7, 0.7, 0.3) |

### Graph Colors
| Series | Color (RGB) |
|--------|-------------|
| Herbivores | (0.3, 0.8, 0.3) |
| Carnivores | (0.9, 0.3, 0.3) |
| Food | (0.8, 0.6, 0.2) |
| Best Fitness | (0.9, 0.7, 0.2) |
| Avg Fitness | (0.3, 0.7, 0.3) |
| Complexity | (0.9, 0.5, 0.5) |

## Keyboard Shortcuts

| Key | Action | Context |
|-----|--------|---------|
| Space | Toggle pause | Global |
| P | Toggle pause | Global |
| F1 | Toggle help | Global |
| F3 | Toggle UI | Global |
| Escape | Deselect creature | When creature selected |
| 1 | Speed 0.5x | Global |
| 2 | Speed 1.0x | Global |
| 3 | Speed 2.0x | Global |
| 4 | Speed 4.0x | Global |
| 5 | Speed 8.0x | Global |

## Responsive Behavior

### Window Resize
- Left panel: Fixed width, full height
- Right panel: Fixed width, adjustable height
- Status bar: Full width, fixed height
- Graphs auto-resize to fill available width

### Panel Collapse
- Headers collapse to save space
- State persisted in ImGui .ini file

## Data Flow

```
SimulationWorld
      |
      v
DashboardMetrics.update()
      |
      +-- updatePopulationCounts()
      +-- updateGeneticDiversity()
      +-- updateEcosystemHealth()
      +-- updateHeatmaps()
      +-- recordHistory() (per second)
      |
      v
SimulationDashboard.render()
      |
      +-- renderLeftPanel() --> Tabs
      +-- renderRightPanel() --> Inspector
      +-- renderStatusBar()
      +-- renderHelpWindow() (if F1)
```

## Implementation Classes

### SimulationDashboard
Main controller for the UI system.

```cpp
class SimulationDashboard {
    // State
    bool m_showDebugPanel;
    bool m_paused;
    float m_simulationSpeed;
    DashboardTab m_currentTab;
    Creature* m_selectedCreature;

    // Components
    DashboardMetrics m_metrics;
    NeuralNetworkVisualizer m_neuralVisualizer;

    // Callbacks
    SpawnCallback m_spawnCreatureCallback;
    FollowCreatureCallback m_followCreatureCallback;
};
```

### DashboardMetrics
Data collection and aggregation.

```cpp
class DashboardMetrics {
    // Population
    std::vector<float> herbivoreHistory;
    std::vector<float> carnivoreHistory;

    // Genetics
    float avgSize, stdSize;
    float geneticDiversity;

    // Ecosystem
    float ecosystemHealth;
    float predatorPreyRatio;
};
```

### NeuralNetworkVisualizer
Neural network topology rendering.

```cpp
class NeuralNetworkVisualizer {
    void render(const NeuralNetwork*, ImVec2 size);
    void renderCompact(const NeuralNetwork*, ImVec2 size);
};
```

## Future Enhancements

1. **Docking Support**
   - Enable ImGuiConfigFlags_DockingEnable
   - Allow panel rearrangement

2. **Layout Persistence**
   - Save custom layouts
   - Quick layout presets

3. **Heatmap Visualization**
   - Population distribution overlay
   - Food density map

4. **Recording/Export**
   - Screenshot capture
   - Metric export to CSV

5. **Creature Selection**
   - Click-to-select in viewport
   - Selection highlight rendering

## Conclusion

This design provides a comprehensive dashboard that:
- Organizes features into logical tabs
- Uses real-time graphs for visualization
- Provides detailed creature inspection
- Maintains consistent visual styling
- Supports keyboard-driven workflow
- Scales appropriately for different screen sizes
