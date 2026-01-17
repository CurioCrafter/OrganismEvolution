cmake_minimum_required(VERSION 3.15)
project(OrganismEvolution VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard (C++20 for std::span)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Windows-only project (DirectX 12)
if(NOT WIN32)
    message(FATAL_ERROR "This project requires Windows for DirectX 12 support")
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DFORGE_DEBUG=1 /DENABLE_DEBUG_LOGGING=1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /DENABLE_DEBUG_LOGGING=0")
    # Enable multi-processor compilation
    add_compile_options(/MP)
    # Disable specific warnings
    add_compile_options(/wd4100)  # Unreferenced formal parameter
    add_compile_options(/wd4189)  # Local variable initialized but not referenced
else()
    # MinGW or other compilers
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DFORGE_DEBUG=1 -DENABLE_DEBUG_LOGGING=1")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DENABLE_DEBUG_LOGGING=0")
endif()

# =============================================================================
# DirectX 12 Build Definitions
# =============================================================================

# Enable DX12 RHI backend
add_compile_definitions(FORGE_RHI_DX12=1)
add_compile_definitions(USE_DX12=1)

# Enable Forge Engine integration for GPU compute
add_compile_definitions(USE_FORGE_ENGINE=1)

# Prevent Windows min/max macro conflicts with std::numeric_limits
add_compile_definitions(NOMINMAX)
add_compile_definitions(WIN32_LEAN_AND_MEAN)

# Enable GLM experimental extensions for animation system
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

# =============================================================================
# Include Directories
# =============================================================================

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/imgui
    ${PROJECT_SOURCE_DIR}/external/implot
    ${PROJECT_SOURCE_DIR}/external/glm
    # ForgeEngine includes
    ${PROJECT_SOURCE_DIR}/ForgeEngine/Core/Public
    ${PROJECT_SOURCE_DIR}/ForgeEngine/RHI/Public
    ${PROJECT_SOURCE_DIR}/ForgeEngine/Platform/Public
    ${PROJECT_SOURCE_DIR}/ForgeEngine/Shaders/Public
)

# =============================================================================
# ForgeEngine RHI Sources (DirectX 12 Implementation)
# =============================================================================

set(FORGE_ENGINE_SOURCES
    # DX12 RHI implementation
    ForgeEngine/RHI/DX12/DX12Device.cpp
    ForgeEngine/RHI/DX12/DX12DebugInterface.cpp
    # Platform (Window management)
    ForgeEngine/Platform/Private/Window.cpp
    ForgeEngine/Platform/Private/Windows/WindowsWindow.cpp
    # Shader compilation
    ForgeEngine/Shaders/Private/ShaderCompiler.cpp
)

# =============================================================================
# ImGui library (DirectX 12 + Win32 backend)
# =============================================================================

set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_impl_win32.cpp
    external/imgui/imgui_impl_dx12.cpp
)

# =============================================================================
# ImPlot library (Plotting for ImGui)
# =============================================================================

set(IMPLOT_SOURCES
    external/implot/implot.cpp
    external/implot/implot_items.cpp
    external/implot/implot_demo.cpp
)

# =============================================================================
# UI Dashboard Sources
# =============================================================================

# UI sources (disabled for minimal GPU steering demo build)
# These have complex dependencies on genetics system
# set(UI_SOURCES
#     src/ui/DashboardMetrics.cpp
#     src/ui/SimulationDashboard.cpp
#     src/ui/NEATVisualizer.cpp
#     src/ui/PhylogeneticTreeVisualizer.cpp
# )
set(UI_SOURCES)

# =============================================================================
# AI / Neural Network Sources
# =============================================================================

set(AI_SOURCES
    src/ai/NeuralNetwork.cpp
    src/ai/NEATGenome.cpp
    src/ai/BrainModules.cpp
    src/ai/CreatureBrainInterface.cpp
    src/ai/GPUSteeringCompute.cpp
)

# =============================================================================
# Terrain System
# NOTE: TerrainChunk.cpp, TerrainChunkManager.cpp, TerrainHeightmapGenerator.cpp
# were REMOVED. They were no-op stubs when USE_FORGE_ENGINE=1 was defined,
# silently returning 0.0f/false/nullptr which caused creatures to fall through terrain.
# Height queries use Terrain class; rendering uses TerrainRendererDX12's procedural generation.
# =============================================================================

set(TERRAIN_SOURCES
    # Empty - chunked terrain was dead code
)

# =============================================================================
# Graphics / Rendering Sources
# =============================================================================

set(GRAPHICS_SOURCES
    src/graphics/Camera.cpp
    src/graphics/CameraController.cpp
    src/graphics/DX12Device.cpp
    src/graphics/Frustum.cpp
    src/graphics/GlobalLighting.cpp
    src/graphics/PostProcess_DX12.cpp
    src/graphics/Shader_DX12.cpp
    src/graphics/ShadowMap_DX12.cpp
    src/graphics/WaterRenderer.cpp
    src/graphics/SkyRenderer.cpp
    src/graphics/TextureAtlas.cpp
    src/graphics/Bioluminescence.cpp
    # Weather effects
    src/graphics/weather/WeatherEffectsManager.cpp
    src/graphics/weather/RainEffect.cpp
    src/graphics/weather/SnowEffect.cpp
    src/graphics/weather/FogSystem.cpp
    src/graphics/weather/WindEffect.cpp
    src/graphics/weather/StormEffect.cpp
    # GPU particle system
    src/graphics/particles/GPUParticleSystem.cpp
    # Rendering
    src/graphics/rendering/CreatureMeshCache.cpp
    src/graphics/rendering/CreatureRenderer_DX12.cpp
    src/graphics/rendering/GrassRenderer_DX12.cpp
    src/graphics/rendering/TreeRenderer_DX12.cpp
    src/graphics/rendering/TerrainRenderer_DX12.cpp
    # Procedural mesh generation
    src/graphics/procedural/MorphologyBuilder.cpp
    src/graphics/procedural/CreatureBuilder.cpp
    src/graphics/procedural/CreatureTextureGenerator.cpp
    src/graphics/procedural/MarchingCubes.cpp
)

# =============================================================================
# Entity Sources (Creatures, Genome)
# =============================================================================

set(ENTITY_SOURCES
    src/entities/Creature.cpp
    src/entities/Genome.cpp
    src/entities/NeuralNetwork.cpp
    src/entities/SteeringBehaviors.cpp
    src/entities/SensorySystem.cpp
    src/entities/CreatureTraits.cpp
    src/entities/EcosystemBehaviors.cpp
    src/entities/SwimBehavior.cpp
    src/entities/NamePhonemeTables.cpp
    src/entities/SpeciesNaming.cpp
    src/entities/SpeciesNameGenerator.cpp
    # Genetics subsystem
    src/entities/genetics/Gene.cpp
    src/entities/genetics/Allele.cpp
    src/entities/genetics/Chromosome.cpp
    src/entities/genetics/DiploidGenome.cpp
    src/entities/genetics/Species.cpp
    src/entities/genetics/HybridZone.cpp
    src/entities/genetics/MateSelector.cpp
    src/entities/genetics/GeneticsManager.cpp
)

# =============================================================================
# Animation Sources
# =============================================================================

set(ANIMATION_SOURCES
    src/animation/ActivityAnimations.cpp
    src/animation/ActivitySystem.cpp
    src/animation/Skeleton.cpp
    src/animation/Pose.cpp
    src/animation/IKSolver.cpp
    src/animation/GaitGenerator.cpp
    src/animation/ProceduralLocomotion.cpp
    src/animation/ProceduralRig.cpp
)

# =============================================================================
# Utility Sources
# =============================================================================

set(UTIL_SOURCES
    src/utils/CommandProcessor.cpp
    src/utils/HierarchicalSpatialGrid.cpp
    src/utils/Random.cpp
    src/utils/PerlinNoise.cpp
    src/utils/SpatialGrid.cpp
)

# =============================================================================
# Environment Sources
# =============================================================================

set(ENVIRONMENT_SOURCES
    src/environment/AquaticPlants.cpp
    src/environment/BiomePalette.cpp
    src/environment/BiomeSystem.cpp
    src/environment/ClimateSystem.cpp
    src/environment/DecomposerSystem.cpp
    src/environment/EcosystemManager.cpp
    src/environment/EcosystemMetrics.cpp
    src/environment/Food.cpp
    src/environment/GrassSystem.cpp
    src/environment/LSystem.cpp
    src/environment/PlanetChemistry.cpp
    src/environment/PlanetSeed.cpp
    src/environment/PlanetTheme.cpp
    src/environment/ProducerSystem.cpp
    src/environment/SeasonManager.cpp
    src/environment/Terrain.cpp
    src/environment/TerrainSampler.cpp
    src/environment/TreeGenerator.cpp
    src/environment/VegetationManager.cpp
    src/environment/WeatherSystem.cpp
)

# =============================================================================
# Core Sources (Simulation orchestration and managers)
# =============================================================================

set(CORE_SOURCES
    src/core/BiochemistrySystem.cpp
    src/core/CreatureManager.cpp
    src/core/CreatureUpdateScheduler.cpp
    src/core/FoodChainManager.cpp
    src/core/GameplayManager.cpp
    src/core/ScanningSystem.cpp
    src/core/SpeciesCatalog.cpp
    # SimulationOrchestrator.cpp is disabled - not used by main.cpp
    # Note: src/core/replay/* files removed - using simpler Forge::ReplaySystem in src/core/ReplaySystem.h instead
)

# =============================================================================
# Audio Sources
# =============================================================================

set(AUDIO_SOURCES
    src/audio/AudioManager.cpp
    src/audio/SoundscapeBudget.cpp
    src/audio/CreatureVoiceGenerator.cpp
    src/audio/AmbientSoundscape.cpp
    src/audio/ProceduralSynthesizer.cpp
)

# =============================================================================
# Main DX12 Entry Point
# =============================================================================

# The src/main.cpp contains EVERYTHING needed for the DX12 build:
# - Complete simulation logic (terrain, creatures, AI, genetics)
# - DX12 renderer using Forge Engine RHI
# - Win32 window management
# - ImGui debug panel
# It's a self-contained ~3600+ line implementation

set(MAIN_SOURCES
    src/main.cpp
)

# =============================================================================
# Combine All Sources
# =============================================================================

set(PHYSICS_SOURCES
    src/physics/Morphology.cpp
    src/physics/Metamorphosis.cpp
)

set(SOURCES
    ${MAIN_SOURCES}
    ${FORGE_ENGINE_SOURCES}
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${UI_SOURCES}
    ${AI_SOURCES}
    ${GRAPHICS_SOURCES}
    ${ENTITY_SOURCES}
    ${ANIMATION_SOURCES}
    ${UTIL_SOURCES}
    ${ENVIRONMENT_SOURCES}
    ${CORE_SOURCES}
    ${TERRAIN_SOURCES}
    ${AUDIO_SOURCES}
    ${PHYSICS_SOURCES}
)

# =============================================================================
# Executable
# =============================================================================

# Create console application (main.cpp uses standard main())
# This also allows debug output in the console
add_executable(${PROJECT_NAME} ${SOURCES})

# =============================================================================
# Link Libraries
# =============================================================================

target_link_libraries(${PROJECT_NAME}
    # DirectX 12
    d3d12
    dxgi
    d3dcompiler
    dxguid
    dxcompiler

    # Windows
    user32
    gdi32
    shell32
    ole32
    uuid
    comdlg32
    advapi32
)

# =============================================================================
# Output Settings
# =============================================================================

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# =============================================================================
# Copy Runtime Shaders to Build Directory
# =============================================================================

# Copy HLSL shaders to build directory for runtime loading
# Note: $<TARGET_FILE_DIR:...> gives the actual output directory (e.g., Debug or Release)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Runtime/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/Runtime/Shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Runtime/Shaders"
    COMMENT "Copying runtime shaders to executable directory"
)

# =============================================================================
# Print Configuration
# =============================================================================

message(STATUS "")
message(STATUS "=== OrganismEvolution DirectX 12 Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "This build uses the standalone ForgeEngine RHI (copied locally)")
message(STATUS "Runtime shaders are located in: ${PROJECT_SOURCE_DIR}/Runtime/Shaders/")
message(STATUS "")
message(STATUS "Build requirements:")
message(STATUS "  - Windows 10 or later")
message(STATUS "  - DirectX 12 compatible GPU")
message(STATUS "  - Windows SDK with DirectX 12 headers")
message(STATUS "  - MSVC 2019+ or compatible compiler with C++20 support")
message(STATUS "")

# =============================================================================
# Test Infrastructure
# =============================================================================

option(BUILD_TESTS "Build test executables" ON)

if(BUILD_TESTS)
    enable_testing()

    # Core library for tests (shared code without main.cpp)
    # Tests need the entity, utility, and animation code
    set(TEST_LIBRARY_SOURCES
        # Entities (core testable units)
        src/entities/Genome.cpp
        src/entities/NeuralNetwork.cpp
        src/entities/Creature.cpp
        src/entities/SteeringBehaviors.cpp
        src/entities/SensorySystem.cpp
        src/entities/CreatureTraits.cpp
        src/entities/EcosystemBehaviors.cpp
        src/entities/SwimBehavior.cpp
        src/entities/FlightBehavior.cpp
        src/entities/SpeciesNaming.cpp
        src/entities/SpeciesNameGenerator.cpp
        # Genetics subsystem
        src/entities/genetics/Gene.cpp
        src/entities/genetics/Allele.cpp
        src/entities/genetics/Chromosome.cpp
        src/entities/genetics/DiploidGenome.cpp
        src/entities/genetics/Species.cpp
        src/entities/genetics/HybridZone.cpp
        src/entities/genetics/MateSelector.cpp
        src/entities/genetics/GeneticsManager.cpp
        # Utilities
        src/utils/Random.cpp
        src/utils/PerlinNoise.cpp
        src/utils/SpatialGrid.cpp
        # Animation
        src/animation/Skeleton.cpp
        src/animation/Pose.cpp
        src/animation/IKSolver.cpp
        src/animation/GaitGenerator.cpp
        src/animation/ProceduralLocomotion.cpp
        src/animation/SwimAnimator.cpp
        # AI (needed for creature brain)
        src/ai/NeuralNetwork.cpp
        src/ai/NEATGenome.cpp
        src/ai/BrainModules.cpp
        src/ai/CreatureBrainInterface.cpp
        # Environment (minimal subset for creature updates)
        src/environment/Terrain.cpp
        src/environment/TerrainSampler.cpp
        src/environment/ClimateSystem.cpp
    )

    # Create static library for test linking
    add_library(organism_core STATIC ${TEST_LIBRARY_SOURCES})
    target_include_directories(organism_core PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/external/glm
    )
    # Define macros needed by test code
    target_compile_definitions(organism_core PUBLIC
        GLM_ENABLE_EXPERIMENTAL
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _USE_MATH_DEFINES
    )

    # =============================================================================
    # Test Executables - Core Unit Tests
    # =============================================================================

    # Genome tests
    add_executable(test_genome tests/test_genome.cpp)
    target_link_libraries(test_genome organism_core)
    add_test(NAME GenomeTests COMMAND test_genome)

    # Neural network tests
    add_executable(test_neural_network tests/test_neural_network.cpp)
    target_link_libraries(test_neural_network organism_core)
    add_test(NAME NeuralNetworkTests COMMAND test_neural_network)

    # Spatial grid tests
    add_executable(test_spatial_grid tests/test_spatial_grid.cpp)
    target_link_libraries(test_spatial_grid organism_core)
    add_test(NAME SpatialGridTests COMMAND test_spatial_grid)

    # Integration tests
    add_executable(test_integration tests/test_integration.cpp)
    target_link_libraries(test_integration organism_core)
    add_test(NAME IntegrationTests COMMAND test_integration)

    # Performance tests
    add_executable(test_performance tests/test_performance.cpp)
    target_link_libraries(test_performance organism_core)
    add_test(NAME PerformanceTests COMMAND test_performance)

    # Serialization tests (save/load round-trip)
    add_executable(test_serialization tests/test_serialization.cpp)
    target_link_libraries(test_serialization organism_core)
    add_test(NAME SerializationTests COMMAND test_serialization)

    # =============================================================================
    # Test Executables - Animation Unit Tests
    # =============================================================================

    # Skeleton tests
    add_executable(test_skeleton tests/animation/test_skeleton.cpp)
    target_link_libraries(test_skeleton organism_core)
    add_test(NAME SkeletonTests COMMAND test_skeleton)

    # Pose tests
    add_executable(test_pose tests/animation/test_pose.cpp)
    target_link_libraries(test_pose organism_core)
    add_test(NAME PoseTests COMMAND test_pose)

    # IK solver tests
    add_executable(test_ik tests/animation/test_ik.cpp)
    target_link_libraries(test_ik organism_core)
    add_test(NAME IKTests COMMAND test_ik)

    # Locomotion tests
    add_executable(test_locomotion tests/animation/test_locomotion.cpp)
    target_link_libraries(test_locomotion organism_core)
    add_test(NAME LocomotionTests COMMAND test_locomotion)

    # Set test output directory
    set_target_properties(
        test_genome test_neural_network test_spatial_grid
        test_integration test_performance test_serialization
        test_skeleton test_pose test_ik test_locomotion
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    message(STATUS "")
    message(STATUS "=== Test Configuration ===")
    message(STATUS "Tests enabled: ON")
    message(STATUS "Test executables will be built in: ${CMAKE_BINARY_DIR}/tests")
    message(STATUS "Run tests with: ctest --output-on-failure")
    message(STATUS "")
endif()
