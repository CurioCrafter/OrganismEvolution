# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

#### Core Simulation Systems
- Complete evolution simulator with genetic algorithms
- Simulation orchestrator with entity management, population balance, and environment systems
- Spatial grid for efficient neighbor queries and collision detection
- Season manager for environmental temporal changes
- Ecosystem manager coordinating producer and decomposer systems

#### Creature Systems
- Genome-based organisms with physical traits (size, speed, efficiency, color)
- Neural network AI brain (4 inputs, 4 hidden neurons, 2 outputs)
- Sensory system with vision, hearing, smell, and touch modalities
- Pheromone communication system (food trails, alarm, mating, territory)
- Spatial memory for food sources, dangers, shelters, and territory
- Multiple creature types with trophic level classification:
  - Herbivores: Grazer, Browser, Frugivore
  - Carnivores: Small Predator, Apex Predator
  - Special: Omnivore, Scavenger, Parasite, Cleaner

#### Steering Behaviors
- Basic behaviors: seek, flee, arrive, wander, pursuit, evasion
- Flocking behaviors: separation, alignment, cohesion
- Environment behaviors: boundary avoidance, obstacle avoidance, terrain following

#### Genetics and Evolution
- Diploid genome with Mendelian inheritance patterns
- Genetic crossover with random trait selection from parents
- Mutation system (10% rate, 15% strength per trait)
- Sexual selection with mate evaluation based on fitness, size, and color
- Species divergence based on genetic distance threshold

#### Energy and Reproduction
- Energy system with multiple food sources (grass, berries, fruits, prey)
- Activity-based energy costs (metabolism, movement, sprinting, attacks)
- Reproduction requirements (energy thresholds, cooldowns, minimum age)
- Population dynamics using Lotka-Volterra predator-prey balance
- Fitness calculation based on survival time, offspring count, and energy efficiency

#### Graphics and Rendering
- DirectX 12 rendering pipeline via ModularGameEngine RHI abstraction
- Multi-pass rendering: Terrain, Vegetation, Creatures, UI/Nametags
- Procedural terrain with height-based biomes (water, sand, grass, rock, snow)
- Instanced creature rendering with per-creature color and animation
- HLSL shaders with Blinn-Phong lighting and exponential fog
- Marching cubes mesh generation for metaball-based creature shapes
- L-System based tree generation

#### Camera and Controls
- First-person camera with WASD movement and mouse look
- Camera zoom via scroll wheel (FOV adjustment 1-90 degrees)
- Tab key to toggle mouse capture for look mode
- Simulation speed controls (+/- keys, range 0.25x to 8.0x)
- Pause/resume functionality (P key)
- Debug command interface via external file

#### Platform Support
- Windows platform with GLFW window management
- OpenGL/GLEW rendering backend
- DLL dependencies bundled for deployment
- MSYS2/MinGW build system with CMake

#### Documentation
- Architecture documentation with system diagrams and data flow
- Creature system documentation with genetics and behavior details
- Rendering pipeline documentation with shader code examples
- Controls reference with input flow diagrams
- Known issues tracking with workarounds and planned fixes

### Fixed
- Delta time spikes on window focus loss (capped to 100ms maximum)
- First mouse movement camera jump (skip first frame after capture)
- DLL dependencies included in build directory for Windows deployment

### Known Issues

#### Critical
- Creatures may not render due to marching cubes mesh generation failing silently when isovalue threshold does not match actual metaball potential values
- Mouse cursor may remain visible on Win32 due to ShowCursor counter desynchronization with GLFW

#### High Priority
- Vertex struct not SIMD-aligned (normal at offset 12 instead of 16)
- Keyboard input processed when window is unfocused (missing focus check)

#### Medium Priority
- Scroll wheel offset accumulation causing erratic zoom
- Vegetation (trees, bushes, grass) spawned but not rendered

## [0.1.0] - 2026-01-11

### Added
- Initial project setup with complete planning and documentation
- Project structure with directories: core, entities, environment, evolution, graphics, utils
- CMake build configuration for MinGW/MSYS2
- Basic OpenGL/GLFW/GLEW application skeleton
- Comprehensive PROJECT_PLAN.md with research, architecture, and implementation phases
- SETUP_GUIDE.md for MSYS2 development environment
- Quick start guide for streamlined setup and development

### Research
- Evolution simulator projects analysis (ALIEN, Keiwan's Evolution, Evolve 4.0)
- Genetic algorithm implementation patterns
- MSYS2 OpenGL setup guides

---

## Configuration Reference

### Simulation Parameters
| Parameter | Value | Description |
|-----------|-------|-------------|
| Initial Herbivores | 60 | Starting herbivore population |
| Initial Carnivores | 15 | Starting carnivore population |
| Max Food Items | 150 | Maximum food entities |
| Mutation Rate | 10% | Probability of gene mutation |
| Mutation Strength | 0.15 | Magnitude of mutations |
| World Size | 300x300 | Terrain grid dimensions |

### Reproduction Parameters
| Creature Type | Energy Threshold | Energy Cost | Cooldown |
|---------------|------------------|-------------|----------|
| Herbivore | 180/200 | 80 | 30 seconds |
| Carnivore | 170/200 | 100 | 45 seconds |

### Render Settings
| Setting | Value |
|---------|-------|
| Resolution | 1280 x 720 |
| MSAA | 4x |
| VSync | Enabled |
| FOV | 45 degrees |
| Near/Far Plane | 0.1 / 1000.0 |
