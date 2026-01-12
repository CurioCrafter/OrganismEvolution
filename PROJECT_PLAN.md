# OrganismEvolution - Evolution Simulator Project Plan

## Project Overview
A 3D evolution simulator featuring creatures that evolve in an environment with islands, oceans, and trees. Organisms with randomly generated traits compete for survival, and successful traits are passed to future generations through genetic algorithms.

## Research Findings

### Similar Projects
Based on research of existing evolution simulators:

1. **[ALIEN (Artificial Life Environment)](https://github.com/chrxh/alien)** - CUDA-powered large-scale simulation with millions of particles
2. **[Evolution by Keiwan](https://keiwan.itch.io/evolution)** - Neural network + genetic algorithm for creature locomotion
3. **[Evolve 4.0](https://github.com/rubberduck203/Evolve)** - 2D universe with creature behavior visualization
4. **[Grovolve](https://github.com/rrwick/Grovolve)** - Educational evolution demonstration
5. **[HTML5 Genetic Algorithm Biped Walkers](https://rednuht.org/genetic_walkers/)** - Web-based walking creature evolution

### Key Concepts to Implement
- **Genetic Algorithms**: Mutation, crossover, selection pressure
- **Neural Networks**: Basic decision-making for creature behavior
- **Physics Simulation**: Movement, collision, energy consumption
- **Environmental Factors**: Food sources, terrain, predator-prey dynamics

## Technology Stack

### Development Environment: MSYS2
MSYS2 provides a Unix-like environment on Windows with MinGW compiler support.

**Installation**: Download from [msys2.org](https://www.msys2.org/)

### Core Technologies
- **Language**: C++17 or later
- **Graphics**: OpenGL + GLFW (window management) + GLEW (extension loading)
- **Math**: GLM (OpenGL Mathematics)
- **Build System**: CMake
- **Version Control**: Git + GitHub

### Required MSYS2 Packages
```bash
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-glfw
pacman -S mingw-w64-x86_64-glew
pacman -S mingw-w64-x86_64-glm
pacman -S mingw-w64-x86_64-mesa
pacman -S git
```

## Architecture Design

### Component Structure
```
OrganismEvolution/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Simulation.cpp/h      # Main simulation loop
│   │   ├── World.cpp/h           # Environment management
│   │   └── Config.cpp/h          # Configuration parameters
│   ├── entities/
│   │   ├── Creature.cpp/h        # Individual organism
│   │   ├── Genome.cpp/h          # Genetic information
│   │   └── NeuralNetwork.cpp/h   # Brain/decision making
│   ├── environment/
│   │   ├── Terrain.cpp/h         # Island/ocean generation
│   │   ├── Vegetation.cpp/h      # Trees and plants
│   │   └── Food.cpp/h            # Food sources
│   ├── evolution/
│   │   ├── GeneticAlgorithm.cpp/h # Evolution logic
│   │   ├── Selection.cpp/h        # Fitness selection
│   │   └── Mutation.cpp/h         # Genetic mutations
│   ├── graphics/
│   │   ├── Renderer.cpp/h        # OpenGL rendering
│   │   ├── Camera.cpp/h          # View control
│   │   ├── Shader.cpp/h          # Shader management
│   │   └── Mesh.cpp/h            # 3D models
│   └── utils/
│       ├── Math.cpp/h            # Helper math functions
│       └── Random.cpp/h          # Random number generation
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
├── assets/
│   └── (textures, models, etc.)
├── docs/
│   └── (documentation)
├── CMakeLists.txt
├── README.md
├── LICENSE
└── .gitignore
```

### Core Systems

#### 1. Simulation Engine
- **Fixed timestep update loop** (physics stability)
- **Delta time management** for rendering
- **Multi-threading support** for creature AI calculations
- **Pause/speed controls**

#### 2. Creature System
**Genome Structure**:
- Size (affects speed, energy consumption)
- Speed (movement rate)
- Vision range (detection distance)
- Energy efficiency (survival time)
- Color/appearance (visual traits)
- Neural network weights (behavior)

**Behavior**:
- Food seeking
- Predator avoidance
- Reproduction when energy threshold met
- Energy management (eating, moving, idle)

**Neural Network**:
- Inputs: Distance to food, distance to predators, energy level, angle to nearest food
- Hidden layers: 1-2 layers
- Outputs: Movement direction, speed multiplier

#### 3. Environment System
**Terrain Generation**:
- Perlin noise for island shapes
- Height-based ocean/land/mountain zones
- Procedural tree placement on land

**Resources**:
- Renewable food sources (plants)
- Energy system (creatures consume food)
- Carrying capacity limits

#### 4. Evolution Mechanics
**Fitness Function**:
- Survival time
- Reproduction count
- Distance traveled (exploration)
- Food consumed

**Selection Methods**:
- Tournament selection
- Elitism (keep top 10% unchanged)
- Roulette wheel selection based on fitness

**Genetic Operations**:
- **Crossover**: Single-point or uniform crossover of parent genes
- **Mutation**: Random trait adjustments (5-10% chance)
- **Mutation rate**: Adaptive based on population diversity

#### 5. Graphics System
**Rendering Pipeline**:
- 3D terrain mesh with height coloring
- Instanced rendering for creatures (performance)
- Simple tree models
- Water shader with basic reflections
- Free-look camera with WASD + mouse

**Visual Features**:
- Color-coded creatures by species/family
- Trail rendering (optional, for movement history)
- Debug overlays (neural network visualization, stats)

## Implementation Phases

### Phase 1: Setup & Basic Rendering (Week 1)
- [ ] Set up MSYS2 environment
- [ ] Initialize Git repository and GitHub connection
- [ ] Create CMake build system
- [ ] Implement basic OpenGL window with GLFW
- [ ] Create simple camera controls
- [ ] Render a test cube or sphere

### Phase 2: Environment (Week 2)
- [ ] Generate procedural terrain with Perlin noise
- [ ] Create ocean/land distinction
- [ ] Implement basic lighting
- [ ] Add simple tree models
- [ ] Create water rendering

### Phase 3: Creature Foundation (Week 3)
- [ ] Design Genome class with traits
- [ ] Implement Creature class with physics
- [ ] Add basic movement (random wandering)
- [ ] Create energy system
- [ ] Add food spawning and consumption

### Phase 4: Neural Networks (Week 4)
- [ ] Implement simple feedforward neural network
- [ ] Connect sensors (vision, energy) to inputs
- [ ] Connect outputs to movement controls
- [ ] Test individual creature AI behavior

### Phase 5: Evolution System (Week 5)
- [ ] Implement reproduction mechanics
- [ ] Create genetic crossover function
- [ ] Add mutation system
- [ ] Implement fitness evaluation
- [ ] Add selection algorithms

### Phase 6: Simulation & Tuning (Week 6)
- [ ] Balance parameters (mutation rates, energy costs)
- [ ] Add statistics tracking (generation, diversity, fitness)
- [ ] Implement pause/resume/speed controls
- [ ] Create UI for stats display
- [ ] Performance optimization

### Phase 7: Polish & Features (Week 7+)
- [ ] Add save/load simulation state
- [ ] Implement interesting scenarios (predator-prey)
- [ ] Create visualization for genealogy
- [ ] Add configuration file support
- [ ] Documentation and README

## Key Parameters to Tune

### Creature Parameters
- Initial population: 50-100 creatures
- Starting energy: 100 units
- Movement cost: 0.5 energy/second while moving
- Reproduction threshold: 150 energy
- Reproduction cost: 50 energy
- Energy from food: 30-50 units

### Evolution Parameters
- Mutation rate: 5-10%
- Mutation magnitude: ±10-20% of trait value
- Crossover rate: 70-80%
- Elite preservation: Top 5-10%
- Generation gap: 50-80% replacement

### Environment Parameters
- World size: 200x200 units
- Food spawn rate: 5-10 items/second
- Max food items: 200
- Tree density: 15-25%

## Success Metrics

1. **Population Stability**: Population doesn't die out or explode
2. **Observable Evolution**: Trait changes visible over 20+ generations
3. **Emergent Behavior**: Creatures develop food-seeking strategies
4. **Performance**: 60 FPS with 100+ creatures
5. **Visual Quality**: Clear, understandable 3D visualization

## Resources & References

### Evolution Simulators
- [ALIEN Project](https://github.com/chrxh/alien) - Advanced CUDA-based simulation
- [Evolution by Keiwan](https://keiwan.itch.io/evolution) - Neural network creature evolution
- [GitHub Evolution Simulation Topics](https://github.com/topics/evolution-simulation)
- [Karl Sims Virtual Creatures Paper](https://www.karlsims.com/papers/siggraph94.pdf)

### Technical Guides
- [Setting up MSYS2 and LibSDL2](https://ncot.uk/blog-posts/setting-up-msys2-and-libsdl2-for-graphics-programming-in-windows/)
- [OpenGL with MSYS2](https://hackmd.io/@yurisoba/rkX9D66ui)
- [Compiling C++/OpenGL on Windows](https://www.jamiebayne.co.uk/blog/opengl-windows)

### Genetic Algorithms
- [Evolving Simple Organisms Tutorial](https://nry.me/posts/2017-11-30/evolving-simple-organisms-using-a-genetic-algorithm-and-deep-learning/)
- [Using Genetic Algorithms to Simulate Evolution](https://arxiv.org/pdf/2209.06822)
- [Training Virtual Creatures](https://www.gamedeveloper.com/programming/training-virtual-creatures-with-reinforcement-learning-and-genetic-algorithms)

## Next Steps

1. Install MSYS2 and required packages
2. Initialize Git repository
3. Create basic CMake project structure
4. Start with Phase 1 implementation

---

**Last Updated**: January 2026
**Project Status**: Planning Phase
