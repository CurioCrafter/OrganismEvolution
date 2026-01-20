# OrganismEvolution

A real-time evolution simulator with neural network creature brains, procedural terrain, and ecosystem simulation. Watch artificial life evolve, hunt, and reproduce in a dynamic 3D world.

## Quick Start

### Prerequisites

- Windows 10/11
- DirectX 12 compatible GPU
- Visual Studio 2022 with C++ workload (or 2019+)
- CMake 3.15+

### Build and Run

Open a **Visual Studio Developer Command Prompt** (this is required!) and run:

```batch
cd C:\Users\andre\Desktop\OrganismEvolution

cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
cmake --build build_dx12 --config Release

build_dx12\Release\OrganismEvolution.exe
```

## Controls

| Key | Action |
|-----|--------|
| W/A/S/D | Move camera |
| Mouse drag | Look around (click to capture) |
| Q/E | Move camera down/up |
| Shift | Move faster |
| P | Pause/Resume simulation |
| F1 | Toggle debug panel |
| F2 | Toggle performance profiler |
| F5 | Quick save |
| F9 | Quick load |
| F10 | Toggle replay mode |
| ESC | Release mouse / Exit |

## Feature Status

### Working Features

| Feature | Status | Notes |
|---------|--------|-------|
| **DirectX 12 Rendering** | Working | GPU-accelerated instanced rendering |
| **Procedural Terrain** | Working | Perlin noise heightmap with water level |
| **Grass Rendering** | Working | Instanced grass with wind animation |
| **Tree Rendering** | Working | L-system procedural trees with LOD |
| **Water Rendering** | Working | Animated water surface with waves |
| **Creature Simulation** | Working | Position, velocity, energy-based simulation |
| **GPU Steering Compute** | Available | Optional compute shader - may need shader path fix |
| **Creature Types** | Working | Herbivore, Carnivore, Aquatic, Flying |
| **ImGui Debug Panel** | Working | Real-time stats and spawn controls |
| **Day/Night Cycle** | Working | Visual lighting changes |
| **Object Pooling** | Working | Memory-efficient creature management |
| **Camera System** | Working | Free, orbit, follow, and cinematic modes |
| **Save/Load System** | Working | F5/F9 quick save, auto-save, save slots |
| **Replay System** | Working | Record and playback simulation |
| **Performance Profiler** | Working | Frame timing breakdown |

### Partial / In Development

| Feature | Status | Notes |
|---------|--------|-------|
| **Genetic Inheritance** | Partial | Genome exists, basic inheritance on reproduction |
| **Neural Network Brains** | Partial | Code exists, not deeply integrated into decisions |
| **Creature Behaviors** | Partial | Basic steering, advanced behaviors scaffolded |
| **Food Chain** | Partial | Basic predator-prey, simplified energy transfer |
| **Procedural Animation** | Partial | Skeletal system exists, not connected to creatures |

### Planned (Not Implemented)

| Feature | Status | Notes |
|---------|--------|-------|
| Interactive UI Dashboard | Planned | Disabled in build due to dependencies |
| Full NEAT Evolution | Planned | Infrastructure only |
| Multi-Island Archipelago | Planned | Headers exist, not integrated |
| Audio System | Planned | Code scaffolded, not connected |
| Climate Effects | Planned | System exists, doesn't affect creatures |
| Natural Disasters | Planned | System exists, not triggered |

## Project Structure

```
OrganismEvolution/
├── src/                      # Active source code
│   ├── main.cpp              # Main entry (~6000 lines, self-contained)
│   ├── entities/             # Creature, Genome, behaviors
│   ├── environment/          # Terrain, Weather, Climate
│   ├── graphics/             # DX12 rendering
│   ├── animation/            # Skeletal animation (partial)
│   ├── ai/                   # Neural networks, GPU steering
│   ├── core/                 # Managers, replay, save
│   ├── physics/              # Collision detection
│   └── ui/                   # ImGui dashboard (disabled)
├── shaders/hlsl/             # HLSL shader source files
├── Runtime/Shaders/          # Compiled shaders (8 files)
├── ForgeEngine/              # DX12 RHI abstraction layer
├── external/                 # Third-party: ImGui, GLM, ImPlot
├── assets/                   # Textures, models, fonts
├── docs/                     # Additional documentation
├── tests/                    # Unit tests (not in build)
├── .github/                  # CI/CD workflows
└── .devcontainer/            # Dev container config
```

**Not tracked** (in .gitignore):
- `build/`, `build_dx12/` - CMake build directories
- `deprecated/` - Old OpenGL code (archived in git history)

## Technology

| Component | Technology |
|-----------|------------|
| Language | C++20 |
| Graphics | DirectX 12 |
| Shaders | HLSL (SM 6.0) |
| Math | GLM |
| UI | Dear ImGui + ImPlot |
| Build | CMake |
| Platform | Windows only |

## System Requirements

### Minimum
- Windows 10 (version 1903+)
- DirectX 12 compatible GPU
- 4GB RAM

### Recommended
- Windows 11
- NVIDIA GTX 1060 / AMD RX 580 or better
- 8GB RAM

## Build Guides

- [QUICK_START.md](QUICK_START.md) - Fast setup (3 commands)
- [BUILD_AND_RUN.md](BUILD_AND_RUN.md) - Detailed build instructions
- [SETUP_GUIDE.md](SETUP_GUIDE.md) - Environment setup
- [KNOWN_ISSUES.md](KNOWN_ISSUES.md) - Known bugs and limitations

## Limitations

- **Windows only** - Requires DirectX 12
- **UI Dashboard disabled** - Complex dependencies, not built
- **Many subsystems scaffolded** - Infrastructure exists but not connected
- **Performance varies** - Depends on creature count and GPU
- **Evolution is basic** - Not full genetic algorithm, simple trait inheritance

## Performance Targets

| Creature Count | Target FPS |
|----------------|-----------|
| 1,000 | 60 FPS |
| 5,000 | 30 FPS |
| 10,000 | 15 FPS |

*Actual performance depends on GPU capability.*

## License

MIT License - see [LICENSE](LICENSE) for details.

---

*Last updated: January 19, 2026*
*Build verified: January 16, 2026*
