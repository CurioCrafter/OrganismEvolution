# UI Research Documentation

## Overview

This document summarizes the research conducted for implementing a comprehensive ImGui dashboard for the OrganismEvolution simulation project.

## ImGui Integration Status

### Current Implementation
- ImGui 1.92+ with DirectX 12 backend
- Win32 platform backend for input handling
- Custom dark theme with blue-tinted colors
- Descriptor heap management for font texture SRV

### Key Files
- `src/ui/ImGuiManager.h/cpp` - Core ImGui initialization and lifecycle
- `src/ui/DashboardMetrics.h/cpp` - Simulation data collection
- `src/ui/NEATVisualizer.h/cpp` - Neural network visualization
- `src/ui/SimulationDashboard.h/cpp` - Main dashboard UI

## ImPlot Integration

### Library Choice
ImPlot was selected for real-time data visualization due to:
- Immediate mode API matching ImGui paradigm
- GPU-accelerated rendering
- Optimized for streaming data (10K-100K points)
- Comprehensive plot types (line, histogram, scatter, heatmap)

### Integration Method
- Added ImPlot source files to `external/implot/`
- Updated CMakeLists.txt to include ImPlot sources
- ImPlot context created/destroyed alongside ImGui context

### Supported Plot Types Used
1. **Line Plots** - Population and fitness over time
2. **Histograms** - Trait distributions (size, speed)
3. **Progress Bars** - Energy, health indicators

## DirectX 12 Backend Details

### Descriptor Management
```cpp
ImGui_ImplDX12_InitInfo initInfo = {};
initInfo.Device = device;
initInfo.CommandQueue = commandQueue;
initInfo.NumFramesInFlight = NUM_FRAMES;
initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
initInfo.SrvDescriptorHeap = srvHeap;
initInfo.LegacySingleSrvCpuDescriptor = cpuHandle;
initInfo.LegacySingleSrvGpuDescriptor = gpuHandle;
```

### Frame Synchronization
- ImGui frame started BEFORE input processing
- This ensures `WantCaptureMouse` is current for click-through prevention
- Render draw data recorded to command list after scene rendering

## Data Collection Architecture

### DashboardMetrics Class
Collects simulation metrics every frame:

| Metric Category | Data Collected |
|-----------------|----------------|
| Population | Herbivore/Carnivore/Aquatic/Flying counts |
| Genetics | Avg/StdDev for size, speed, vision, efficiency |
| Ecosystem | Predator-prey ratio, food availability, health score |
| Performance | FPS, frame time history (120 samples) |
| History | 300-sample rolling history for graphs |

### Update Frequency
- Population counts: Every frame
- History recording: Once per second
- Birth/death rates: Accumulated per minute
- Heatmaps: Every frame (20x20 grid)

## UI Panel Research

### Existing Debug Panel Features
1. Performance (FPS, frame time)
2. Population counts with H/C ratio bar
3. Camera position/rotation controls
4. Simulation pause/speed controls
5. Day/Night cycle controls
6. Spawn controls
7. Basic neural network stats
8. Evolution stats per type
9. Graphics toggles
10. Controls help

### Missing Features Identified
- Real-time population graphs
- Trait distribution visualizations
- Full genetic diversity metrics
- Ecosystem health indicators
- NEAT neural network topology display
- Creature selection/inspection
- Species phylogeny tracking
- Heatmap visualization
- Memory profiling display

## Neural Network Visualization

### Approach
- Layered node layout algorithm
- Color-coded node types (Input=Blue, Hidden=Green, Output=Red, Bias=Yellow)
- Weight-colored connections (Green=positive, Red=negative)
- Thickness proportional to weight magnitude

### Performance Considerations
- Compact mode for sidebar display (6px nodes, no weight labels)
- Full mode for detailed inspection (12px nodes, weight labels)
- Cached layout calculations

## Widget Design Patterns

### Collapsible Headers
Used for panel organization with DefaultOpen for important sections.

### Progress Bars
- Energy/health indicators
- Herbivore/Carnivore ratio
- Ecosystem health score

### Color Coding
- Green: Healthy/positive values
- Yellow: Warning/neutral states
- Red: Critical/negative values

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| Space/P | Toggle pause |
| F1 | Show/hide help |
| F3 | Toggle UI panels |
| Escape | Deselect creature |
| 1-5 | Set simulation speed |

## Status Bar Design
Fixed position at bottom of viewport:
```
FPS: 60 | Creatures: 1000 | Gen: 50 | Time: 12:34 | Health: 75% | RUNNING (1.0x)
```

## Performance Optimizations

### Frame Rate Targets
- UI rendering: < 1ms overhead target
- Graph updates: Circular buffers prevent allocation
- History trimming: Automatic at HISTORY_SIZE (300 samples)

### Memory Management
- Pre-reserved vector capacity
- Fixed-size circular buffers
- Lazy evaluation for complex metrics

## References

### ImGui Documentation
- https://github.com/ocornut/imgui
- https://github.com/ocornut/imgui/wiki

### ImPlot Documentation
- https://github.com/epezent/implot
- https://github.com/epezent/implot_demos

### DirectX 12 ImGui Backend
- https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx12.cpp
- https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx12/

## Conclusion

The research phase identified all necessary components for implementing a comprehensive dashboard:
1. ImPlot for real-time graphs
2. Tabbed interface for organization
3. Creature inspector with genome/brain details
4. Status bar for at-a-glance information
5. Keyboard shortcuts for efficiency

All technical requirements have been validated and the implementation follows established ImGui patterns for DirectX 12 applications.
