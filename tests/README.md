# OrganismEvolution Test Suite

This directory contains comprehensive tests for the OrganismEvolution simulation.

## Test Categories

### Core Unit Tests

| Test File | Description | What's Tested |
|-----------|-------------|---------------|
| `test_genome.cpp` | Genetic system tests | Trait defaults, randomization, mutation, crossover, neural weights, flying traits, aquatic traits, sensory traits |
| `test_neural_network.cpp` | Neural network tests | Creation, forward pass, determinism, weight sensitivity, input sensitivity, edge cases, behavior modulation |
| `test_spatial_grid.cpp` | Spatial partitioning tests | Grid creation, insertion, radius queries, type filtering, boundary conditions, performance |
| `test_integration.cpp` | System integration tests | Creature lifecycle, genetic inheritance, type traits, save/load structures, animation initialization |
| `test_performance.cpp` | Performance benchmarks | 1K-10K creature scaling, spatial queries, neural forward pass, genome mutation/crossover timing |
| `test_serialization.cpp` | Save/load round-trip | Binary primitives, header serialization, creature/food/world data, multi-creature round-trip, error handling |

### Animation Unit Tests (tests/animation/)

| Test File | Description | What's Tested |
|-----------|-------------|---------------|
| `test_skeleton.cpp` | Skeleton hierarchy | Bone transforms, parent/child relationships, factory methods for different creature types |
| `test_pose.cpp` | Animation poses | Skin weights, bind pose, global transforms, pose blending, skinning matrices |
| `test_ik.cpp` | Inverse kinematics | Two-bone IK, FABRIK, CCD, IK system management, pole vectors |
| `test_locomotion.cpp` | Procedural animation | Gait presets, phase updates, foot placements, wing/spine configuration, creature animators |

## Running Tests

### Using CMake/CTest

```bash
# Configure with tests enabled (default)
cmake -B build -DBUILD_TESTS=ON

# Build all tests
cmake --build build

# Run all tests
cd build
ctest --output-on-failure

# Run specific test category
ctest -R GenomeTests --output-on-failure
ctest -R NeuralNetworkTests --output-on-failure
ctest -R SpatialGridTests --output-on-failure
ctest -R IntegrationTests --output-on-failure
ctest -R PerformanceTests --output-on-failure
ctest -R SerializationTests --output-on-failure

# Run with verbose output
ctest -V --output-on-failure
```

### Running Individual Test Executables

```bash
# Run directly from build directory
build/tests/Release/test_genome.exe
build/tests/Release/test_neural_network.exe
build/tests/Release/test_spatial_grid.exe
build/tests/Release/test_integration.exe
build/tests/Release/test_performance.exe
```

## Performance Targets

| Test | Target | Description |
|------|--------|-------------|
| 1000 Creatures | <16ms | 60 FPS with 1000 creatures |
| 2000 Creatures | <16ms | 60 FPS with 2000 creatures |
| 5000 Creatures | <33ms | 30 FPS with 5000 creatures |
| 10000 Creatures | <100ms | 10 FPS with 10000 creatures |
| Spatial Query | <100μs | Per-query time |
| Neural Forward | <10μs | Per-creature neural pass |
| Genome Mutation | <5μs | Per-mutation time |
| Genome Crossover | <10μs | Per-crossover time |

## Test Coverage

### What's Tested

**Genome System:**
- Default trait values and bounds
- Randomization produces valid, distinct values
- Mutation changes traits within bounds
- Crossover produces valid offspring
- Neural weight inheritance
- Flying/aquatic/sensory traits

**Neural Network:**
- Forward pass produces bounded outputs
- Deterministic behavior (same inputs = same outputs)
- Weight and input sensitivity
- Edge cases (zeros, extremes, negative values)
- Legacy process() interface
- Behavior modulation outputs

**Spatial Grid:**
- Insertion and querying
- Type-based filtering
- Boundary handling
- Performance with large populations
- Statistics tracking

**Integration:**
- Creature creation and lifecycle
- Genetic inheritance across generations
- Type-specific traits (flying, aquatic, predator)
- Save/load data structures
- Animation system initialization

### What's NOT Tested (Requires Full Simulation)

- Actual creature movement and behavior updates (requires terrain)
- Rendering and graphics systems (requires GPU)
- Full save/load round-trip (requires file I/O permissions)
- Sound and audio systems
- UI components
- Multi-threading and parallel update systems

## Adding New Tests

1. Create a new test file in `tests/` or `tests/animation/`
2. Include necessary headers with project-relative paths:
   ```cpp
   #include "entities/Creature.h"
   #include "entities/Genome.h"
   ```
3. Use `assert()` for test conditions
4. Print progress with `std::cout`
5. Return 0 on success
6. Add to CMakeLists.txt:
   ```cmake
   add_executable(test_mytest tests/test_mytest.cpp)
   target_link_libraries(test_mytest organism_core)
   add_test(NAME MyTests COMMAND test_mytest)
   ```

## CI Integration

Tests run automatically on:
- Every push to `main` and `develop`
- Every pull request to `main` and `develop`

CI workflow: `.github/workflows/test.yml`

## Troubleshooting

**Tests don't compile:**
- Ensure `BUILD_TESTS=ON` in CMake configuration
- Check that all source files are in `organism_core` library

**Tests fail at runtime:**
- Check for API changes in headers
- Verify trait bounds haven't changed
- For performance tests, ensure Release build

**CTest doesn't find tests:**
- Rebuild with `cmake --build build`
- Check `build/tests/` for executables
