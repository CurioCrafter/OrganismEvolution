# ImGui Debug Panel Documentation

## Overview

The ImGui-based debug panel provides real-time visualization and control of the evolution simulation. It offers comprehensive monitoring of performance metrics, creature populations, and simulation control through an intuitive graphical interface.

## Accessing the Panel

- **Toggle Visibility**: Press the **backtick key (`)** to show/hide the dashboard
- **Menu Bar**: Use the "Panels" menu to enable/disable individual panels

## Panel Descriptions

### 1. Performance & Debug Panel

The main debug panel with comprehensive monitoring and control features.

#### Frame Rate Section
- **FPS Display**: Current frames per second with color coding:
  - Green: >= 60 FPS (optimal)
  - Yellow: 30-60 FPS (acceptable)
  - Red: < 30 FPS (needs attention)

- **Frame Time Statistics**:
  - Current average frame time in milliseconds
  - Minimum and maximum frame times

- **Frame Time Graph**:
  - Displays the last 120 frames as a line graph
  - Cyan-colored plot for visibility
  - Auto-scaling based on maximum frame time
  - Reference lines for 60 FPS (16.67ms) and 30 FPS (33.33ms)

#### Memory Usage Section
- **Working Set Display**: Shows current memory usage in MB
- **Visual Progress Bar**: Color-coded memory usage indicator:
  - Green: < 50% of 1GB reference
  - Yellow: 50-70%
  - Red: > 70%

Platform-specific implementation:
- **Windows**: Uses PSAPI for process memory info
- **Linux**: Reads from /proc/self/statm
- **macOS**: Uses Mach task_info

#### Creature Counts Section
- **Population Display**:
  - Herbivores (green)
  - Carnivores (red)
  - Food items (yellow)
  - Total entity count

- **Population Balance Bar**: Visual stacked bar showing herbivore/carnivore ratio
  - Green portion: Herbivores
  - Red portion: Carnivores
  - Percentage breakdown displayed below

#### Simulation Control Section
- **Speed Display**: Current simulation speed multiplier
- **Speed Slider**: Adjustable from 0.1x to 5.0x
- **Quick Speed Buttons**:
  - 0.5x, 1x, 2x, 4x presets for fast access
- **Pause Checkbox**: Toggle simulation pause state

### 2. Camera Info Panel

Displays real-time camera position and orientation.

- **Position**: X, Y, Z coordinates in world space
- **Orientation**: Pitch and Yaw angles in degrees
- **Look Direction**: Normalized front vector
- **FOV**: Current field of view
- **Reset Button**: Returns camera to default position (0, 80, 150)

### 3. Population History Panel

Historical graphs of population over time.

- **Herbivore History**: Green line plot
- **Carnivore History**: Red line plot
- **Food History**: Yellow line plot
- Updates once per second
- Displays last 300 samples (5 minutes of data)

### 4. Genetic Diversity Panel

Tracks genetic variation in the population.

- **Trait Statistics Table**:
  - Size, Speed, Vision, Efficiency
  - Average and standard deviation for each

- **Diversity Score**: Combined metric (0-100%)
  - Green: >= 70%
  - Yellow: 50-70%
  - Orange: 30-50%
  - Red: < 30% (warning displayed)

### 5. Ecosystem Health Panel

Overall ecosystem status indicators.

- **Predator/Prey Ratio**: Progress bar with status text
- **Average Creature Age & Energy**
- **Food Availability Ratio**
- **Birth/Death Rates**: Events per minute
- **Generation Info**: Max and average generation
- **Health Score**: 0-100 composite score

### 6. Creature Inspector Panel

Detailed view of a selected creature.

- **Selection**: Press **I** key or click to select creature under cursor
- **Displays**:
  - ID and type (Herbivore/Carnivore)
  - Generation number
  - Health and energy bars
  - Age
  - Genome traits (size, speed, vision)
  - Color preview
  - Position and velocity
  - Fitness score
  - Fear level
  - Kill count (carnivores only)

### 7. Population Heatmap Panel

Spatial distribution visualization.

- **20x20 Grid**: Covers the entire world area
- **Toggle**: Switch between creature and food density
- **Color Coding**:
  - Creature heatmap: Blue (low) to Red (high)
  - Food heatmap: Yellow intensity

## Technical Implementation

### Frame Time Circular Buffer

```cpp
static constexpr int FRAME_TIME_HISTORY_SIZE = 120;
float frameTimeHistory[FRAME_TIME_HISTORY_SIZE];
int frameTimeIndex = 0;  // Current write position
```

The circular buffer stores 120 frame times (approximately 2 seconds at 60 FPS). The `recordFrameTime()` function updates the buffer and calculates min/max/average statistics.

### Memory Usage Detection

Platform-specific implementations:
- **Windows**: `GetProcessMemoryInfo()` with `PROCESS_MEMORY_COUNTERS_EX`
- **Linux**: Parse `/proc/self/statm` for resident set size
- **macOS**: `task_info()` with `MACH_TASK_BASIC_INFO`

### Simulation Speed Control

The UI speed slider syncs bidirectionally with the simulation:
1. UI reads current simulation speed each frame
2. Changes to UI slider are detected and applied to simulation
3. Speed is clamped to 0.1x - 5.0x range

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| ` (backtick) | Toggle dashboard visibility |
| I | Inspect creature under cursor |
| P | Pause/Resume simulation |
| +/- | Increase/Decrease speed |
| F1 | Cycle debug shader modes |
| F2 | Toggle test mode |
| F3 | Toggle position markers |
| F4 | Toggle camera-to-creature line |
| F5 | Print creature debug info |

## Performance Considerations

- **Memory Usage Update**: Called every frame but platform calls are relatively lightweight
- **Frame Time Recording**: O(1) circular buffer insertion
- **Heatmap Update**: O(n) where n = creature count, runs each frame
- **History Recording**: Runs once per second, O(1) per update
- **ImGui Overhead**: Minimal when panels are collapsed or hidden

## Files Modified

- `src/ui/DashboardUI.h` - Panel declarations and state variables
- `src/ui/DashboardUI.cpp` - Rendering implementation
- `src/ui/DashboardMetrics.h` - Frame time buffer and memory tracking
- `src/ui/DashboardMetrics.cpp` - Platform-specific memory detection
- `src/core/Simulation.h` - Speed getter/setter methods
- `src/main.cpp` - Integration of new metric updates
