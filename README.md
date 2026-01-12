# OrganismEvolution

A 3D evolution simulator where creatures with randomly generated traits compete for survival in an environment with islands, oceans, and trees. Successful organisms pass their genetic traits to future generations through natural selection.

## Features

- **Procedural Environment**: Island terrain with oceans and vegetation generated using Perlin noise
- **Genetic Algorithms**: Creatures evolve through mutation, crossover, and natural selection
- **Neural Networks**: Basic AI for creature decision-making and behavior
- **3D Visualization**: Real-time OpenGL rendering with free-look camera
- **Emergent Behavior**: Watch creatures develop food-seeking and survival strategies

## Technology Stack

- **Language**: C++17
- **Graphics**: OpenGL, GLFW, GLEW
- **Math**: GLM (OpenGL Mathematics)
- **Build System**: CMake
- **Development Environment**: MSYS2 (Windows)

## Prerequisites

### MSYS2 Installation (Windows)

1. Download and install MSYS2 from [msys2.org](https://www.msys2.org/)
2. Open MSYS2 MinGW 64-bit terminal
3. Update package database:
   ```bash
   pacman -Syu
   ```
4. Install required packages:
   ```bash
   pacman -S mingw-w64-x86_64-gcc \
             mingw-w64-x86_64-cmake \
             mingw-w64-x86_64-glfw \
             mingw-w64-x86_64-glew \
             mingw-w64-x86_64-glm \
             mingw-w64-x86_64-mesa \
             git \
             make
   ```

## Building the Project

```bash
# Clone the repository
git clone https://github.com/CurioCrafter/OrganismEvolution.git
cd OrganismEvolution

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "MinGW Makefiles"

# Build
cmake --build .

# Run
./OrganismEvolution
```

## Controls

- **WASD**: Move camera
- **Mouse**: Look around
- **Space**: Pause/Resume simulation
- **+/-**: Increase/Decrease simulation speed
- **ESC**: Exit

## How It Works

### Creatures
Each creature has a genome containing:
- Size (affects speed and energy consumption)
- Speed (movement rate)
- Vision range (detection distance)
- Energy efficiency (survival duration)
- Neural network weights (behavior patterns)

### Evolution Process
1. Creatures start with random genomes
2. They consume food to gain energy
3. When energy exceeds a threshold, they reproduce
4. Offspring inherit traits from parents with mutations
5. Less fit creatures die out over generations
6. Population adapts to environmental pressures

### Neural Network
Creatures use a simple feedforward neural network:
- **Inputs**: Distance to food, distance to threats, current energy, direction to nearest food
- **Hidden Layers**: 1-2 layers for processing
- **Outputs**: Movement direction and speed

## Project Structure

```
OrganismEvolution/
├── src/              # Source code
│   ├── core/         # Simulation engine
│   ├── entities/     # Creatures and genomes
│   ├── environment/  # Terrain, trees, food
│   ├── evolution/    # Genetic algorithms
│   ├── graphics/     # Rendering system
│   └── utils/        # Helper functions
├── shaders/          # GLSL shaders
├── docs/             # Documentation
├── CMakeLists.txt    # Build configuration
└── README.md         # This file
```

## Roadmap

- [x] Project planning and research
- [ ] Basic OpenGL rendering setup
- [ ] Procedural terrain generation
- [ ] Creature physics and movement
- [ ] Neural network implementation
- [ ] Genetic algorithm and evolution
- [ ] Statistics and visualization
- [ ] Save/load functionality
- [ ] Performance optimization

## Inspiration

This project draws inspiration from:
- [ALIEN](https://github.com/chrxh/alien) - CUDA-powered artificial life simulation
- [Evolution by Keiwan](https://keiwan.itch.io/evolution) - Neural network creature evolution
- [Karl Sims' Virtual Creatures](https://www.karlsims.com/papers/siggraph94.pdf) - Pioneering work in evolutionary simulation

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

MIT License - see LICENSE file for details

## Author

CurioCrafter

---

**Status**: In Development
**Last Updated**: January 2026
