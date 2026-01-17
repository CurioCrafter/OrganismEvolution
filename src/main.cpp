/*
 * OrganismEvolution - Evolution Simulator
 * DirectX 12 Entry Point with ForgeEngine RHI
 *
 * Features:
 * - GPU-accelerated steering behaviors via compute shaders (when available)
 * - CPU fallback for steering when GPU compute fails
 * - ImGui debug panel for real-time monitoring
 * - GPU steering status clearly shown in console and UI
 */

// Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// DirectX 12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>

// Standard library
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <vector>
#include <array>
#include <memory>
#include <chrono>
#include <random>
#include <thread>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <future>
#include <mutex>
#include <fstream>
#include <filesystem>

// ForgeEngine
#include "Platform/Window.h"
#include "RHI/RHI.h"
#include "Core/CoreMinimal.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// GLM for math (still used by entities)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// GPU Steering Compute
#include "ai/GPUSteeringCompute.h"
#include "ai/DX12DeviceAdapter.h"

// Entities and simulation (headers only for now - full integration later)
#include "entities/CreatureType.h"
#include "entities/Genome.h"
#include "entities/Creature.h"

// Procedural creature mesh cache
#include "graphics/rendering/CreatureMeshCache.h"

// Replay System (S-05, S-06, S-07, S-08 fixes)
#include "core/ReplaySystem.h"

// Save/Load System (Phase 4 bug fix - integration)
#include "core/SaveManager.h"
// Creature Manager (Phase 10)
#include "core/CreatureManager.h"
// Day/Night Cycle
#include "core/DayNightCycle.h"

// Gameplay Systems (Agent 26 - Fun Factor)
#include "core/GameplayManager.h"
#include "ui/GameplayUI.h"
#include "core/SimulationOrchestrator.h"

// Phase 10 Observer UI (Agent 8)
#include "ui/SelectionSystem.h"
#include "ui/CreatureInspectionPanel.h"
#include "ui/MainMenu.h"
#include "ui/GodModeUI.h"

// Behavior systems
#include "entities/behaviors/BehaviorCoordinator.h"

// Water Rendering System (Phase 5 - 3D World)
#include "graphics/WaterRenderer.h"

// Grass Rendering System (Phase 5 - 3D World)
#include "graphics/rendering/GrassRenderer_DX12.h"
#include "environment/GrassSystem.h"
#include "environment/TerrainSampler.h"

// Tree Rendering System (Phase 5 - 3D World)
#include "graphics/rendering/TreeRenderer_DX12.h"
#include "environment/VegetationManager.h"
#include "environment/Terrain.h"
#include "environment/ProceduralWorld.h"

// Climate/Weather
#include "environment/ClimateSystem.h"
#include "environment/SeasonManager.h"
#include "environment/WeatherSystem.h"

// Terrain Rendering System (Phase 5 - 3D World)
#include "graphics/rendering/TerrainRenderer_DX12.h"

// Camera (for selection system)
#include "graphics/Camera.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace Forge;
using namespace Forge::RHI;
using namespace Forge::Platform;

// ============================================================================
// Forward declare ImGui Win32 handler
// ============================================================================
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// Configuration Constants
// ============================================================================
constexpr u32 WINDOW_WIDTH = 1920;
constexpr u32 WINDOW_HEIGHT = 1080;
#ifndef FRAME_COUNT_DEFINED
#define FRAME_COUNT_DEFINED
constexpr u32 FRAME_COUNT = 2;
#endif
constexpr u32 MAX_SRV_DESCRIPTORS = 256;
constexpr u32 IMGUI_SRV_INDEX = 0;
constexpr float CAMERA_FOV_DEGREES = 60.0f;
constexpr float CREATURE_GROUND_CLEARANCE = 0.05f;
constexpr size_t MAX_CREATURE_MESHES_DX12 = 256;

// GPU steering thresholds
constexpr u32 GPU_STEERING_THRESHOLD = 200;  // Use GPU when creature count exceeds this
constexpr u32 MAX_CREATURES = 65536;
constexpr u32 MAX_FOOD_SOURCES = 4096;

// ============================================================================
// Simple Creature Data (for GPU compute integration)
// ============================================================================
static CreatureType GetRenderBaseType(CreatureType type) {
    if (isAquatic(type)) return CreatureType::AQUATIC;
    if (isFlying(type)) return CreatureType::FLYING;
    if (isHerbivore(type)) return CreatureType::HERBIVORE;
    return CreatureType::CARNIVORE;
}

static void InitializeGenomeForType(Genome& genome, CreatureType type) {
    CreatureType baseType = GetRenderBaseType(type);

    if (baseType == CreatureType::AQUATIC) {
        if (type == CreatureType::AQUATIC_APEX) {
            genome.randomizeShark();
        } else if (isAquaticPredator(type)) {
            genome.randomizeAquaticPredator();
        } else {
            genome.randomizeAquatic();
        }
        return;
    }

    if (baseType == CreatureType::FLYING) {
        if (type == CreatureType::AERIAL_PREDATOR) {
            genome.randomizeAerialPredator();
        } else if (isBirdType(type)) {
            genome.randomizeBird();
        } else if (isInsectType(type)) {
            genome.randomizeInsect();
        } else {
            genome.randomizeFlying();
        }
        return;
    }

    genome.randomize();

    if (baseType == CreatureType::CARNIVORE) {
        genome.size = std::clamp(genome.size * 1.2f, 0.5f, 2.0f);
        genome.speed = std::clamp(genome.speed * 1.1f, 5.0f, 20.0f);
    }
}

static glm::vec3 GetTypeTint(CreatureType type) {
    CreatureType baseType = GetRenderBaseType(type);
    switch (baseType) {
        case CreatureType::HERBIVORE:
            return glm::vec3(0.2f, 0.85f, 0.3f);
        case CreatureType::CARNIVORE:
            return glm::vec3(0.9f, 0.2f, 0.2f);
        case CreatureType::AQUATIC:
            return glm::vec3(0.2f, 0.7f, 0.9f);
        case CreatureType::FLYING:
            return glm::vec3(0.95f, 0.7f, 0.2f);
        default:
            return glm::vec3(0.8f, 0.8f, 0.8f);
    }
}

struct SimCreature {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 facing;
    float energy;
    float fear;
    CreatureType type;
    Genome genome;
    bool alive;
    u32 id;
    bool pooled;  // Indicates if this creature is managed by a pool
    u32 poolIndex;
    u32 activeListIndex;

    SimCreature()
        : position(0.0f), velocity(0.0f), facing(0.0f, 0.0f, 1.0f),
          energy(100.0f), fear(0.0f),
          type(CreatureType::HERBIVORE), alive(false), id(0), pooled(false),
          poolIndex(0), activeListIndex(0) {
        InitializeGenomeForType(genome, type);
    }

    SimCreature(const glm::vec3& pos, CreatureType t, u32 creatureId)
        : position(pos), velocity(0.0f), facing(0.0f, 0.0f, 1.0f),
          energy(100.0f), fear(0.0f),
          type(t), alive(true), id(creatureId), pooled(false),
          poolIndex(0), activeListIndex(0) {
        InitializeGenomeForType(genome, type);
    }

    // Reset creature to initial state for reuse from pool
    void reset(const glm::vec3& pos, CreatureType t, u32 creatureId, float initialEnergy = 100.0f) {
        position = pos;
        velocity = glm::vec3(0.0f);
        facing = glm::vec3(0.0f, 0.0f, 1.0f);
        energy = initialEnergy;
        fear = 0.0f;
        type = t;
        alive = true;
        id = creatureId;
        InitializeGenomeForType(genome, type);
        // pooled flag is not reset - it's managed by the pool
    }
};

struct Food {
    glm::vec3 position;
    float amount;

    Food(const glm::vec3& pos, float amt) : position(pos), amount(amt) {}
};

// ============================================================================
// Creature Pool - Object pooling for efficient memory management
// ============================================================================
class CreaturePool {
public:
    // Configuration
    static constexpr u32 DEFAULT_INITIAL_SIZE = 2048;
    static constexpr u32 GROWTH_FACTOR = 2;
    static constexpr u32 MAX_POOL_SIZE = MAX_CREATURES;

    CreaturePool(u32 initialSize = DEFAULT_INITIAL_SIZE)
        : m_nextId(0), m_activeCount(0), m_peakActiveCount(0),
          m_totalAcquisitions(0), m_totalReleases(0), m_poolGrowthCount(0) {
        preallocate(initialSize);
    }

    ~CreaturePool() = default;

    // Acquire a creature from the pool
    // Returns nullptr if pool is exhausted and cannot grow
    SimCreature* acquire(const glm::vec3& position, CreatureType type, float energy = 100.0f) {
        SimCreature* creature = nullptr;

        if (!m_availableIndices.empty()) {
            // Reuse from pool
            u32 index = m_availableIndices.back();
            m_availableIndices.pop_back();
            creature = m_pool[index].get();
            creature->reset(position, type, m_nextId++, energy);
            creature->poolIndex = index;
            creature->activeListIndex = static_cast<u32>(m_activeIndices.size());
            m_activeIndices.push_back(index);
        } else {
            // Pool exhausted - try to grow
            if (m_pool.size() < MAX_POOL_SIZE) {
                grow();
                if (!m_availableIndices.empty()) {
                    u32 index = m_availableIndices.back();
                    m_availableIndices.pop_back();
                    creature = m_pool[index].get();
                    creature->reset(position, type, m_nextId++, energy);
                    creature->poolIndex = index;
                    creature->activeListIndex = static_cast<u32>(m_activeIndices.size());
                    m_activeIndices.push_back(index);
                }
            }
        }

        if (creature) {
            m_activeCount++;
            m_totalAcquisitions++;
            if (m_activeCount > m_peakActiveCount) {
                m_peakActiveCount = m_activeCount;
            }
        }

        return creature;
    }

    // Release a creature back to the pool
    void release(SimCreature* creature) {
        if (!creature || !creature->pooled) return;

        u32 poolIndex = creature->poolIndex;
        if (poolIndex >= m_pool.size() || m_pool[poolIndex].get() != creature) {
            return;
        }

        // Ensure this creature is still tracked as active
        if (creature->activeListIndex >= m_activeIndices.size() ||
            m_activeIndices[creature->activeListIndex] != poolIndex) {
            return;
        }

        creature->alive = false;
        m_availableIndices.push_back(poolIndex);
        removeActiveIndex(creature->activeListIndex);
        m_activeCount = static_cast<u32>(m_activeIndices.size());
        m_totalReleases++;
    }

    // Release creature by checking alive status - used during update loop
    void releaseDeadCreatures() {
        for (u32 i = 0; i < m_activeIndices.size(); ) {
            u32 poolIndex = m_activeIndices[i];
            SimCreature* creature = m_pool[poolIndex].get();

            if (!creature->alive) {
                m_availableIndices.push_back(poolIndex);
                removeActiveIndex(i);
                m_totalReleases++;
                continue;
            }

            ++i;
        }

        m_activeCount = static_cast<u32>(m_activeIndices.size());
    }

    // Get all active creatures (for iteration)
    std::vector<SimCreature*> getActiveCreatures() const {
        std::vector<SimCreature*> active;
        active.reserve(m_activeIndices.size());
        for (u32 index : m_activeIndices) {
            SimCreature* creature = m_pool[index].get();
            if (creature->alive) {
                active.push_back(creature);
            }
        }
        return active;
    }

    // Statistics and monitoring
    u32 getActiveCount() const { return static_cast<u32>(m_activeIndices.size()); }
    u32 getAvailableCount() const { return static_cast<u32>(m_availableIndices.size()); }
    u32 getTotalCapacity() const { return static_cast<u32>(m_pool.size()); }
    u32 getPeakActiveCount() const { return m_peakActiveCount; }
    u32 getTotalAcquisitions() const { return m_totalAcquisitions; }
    u32 getTotalReleases() const { return m_totalReleases; }
    u32 getPoolGrowthCount() const { return m_poolGrowthCount; }
    float getUtilization() const {
        return m_pool.empty() ? 0.0f :
            static_cast<float>(m_activeIndices.size()) / static_cast<float>(m_pool.size());
    }

    // Reset pool statistics (keeps creatures)
    void resetStats() {
        m_peakActiveCount = m_activeCount;
        m_totalAcquisitions = 0;
        m_totalReleases = 0;
        m_poolGrowthCount = 0;
    }

    // Clear entire pool
    void clear() {
        m_availableIndices.clear();
        m_activeIndices.clear();
        for (u32 i = 0; i < m_pool.size(); ++i) {
            m_pool[i]->alive = false;
            m_availableIndices.push_back(i);
        }
        m_activeCount = 0;
        m_nextId = 0;
    }

    // Set the next ID counter (used when loading saves to prevent ID collisions)
    void setNextId(u32 nextId) {
        m_nextId = nextId;
    }

    // Get current next ID value
    u32 getNextId() const {
        return m_nextId;
    }

private:
    // Pre-allocate creatures
    void preallocate(u32 count) {
        u32 startIndex = static_cast<u32>(m_pool.size());
        m_pool.reserve(startIndex + count);
        m_availableIndices.reserve(m_availableIndices.size() + count);

        for (u32 i = 0; i < count; ++i) {
            auto creature = std::make_unique<SimCreature>();
            creature->pooled = true;
            creature->alive = false;
            creature->poolIndex = startIndex + i;
            m_pool.push_back(std::move(creature));
            m_availableIndices.push_back(startIndex + i);
        }

        std::cout << "CreaturePool: Preallocated " << count << " creatures (total: " << m_pool.size() << ")" << std::endl;
    }

    // Grow pool when exhausted
    void grow() {
        u32 currentSize = static_cast<u32>(m_pool.size());
        u32 growthAmount = std::min(currentSize * (GROWTH_FACTOR - 1), MAX_POOL_SIZE - currentSize);

        if (growthAmount > 0) {
            preallocate(growthAmount);
            m_poolGrowthCount++;
            std::cout << "CreaturePool: Pool grew to " << m_pool.size() << " creatures" << std::endl;
        }
    }

    std::vector<std::unique_ptr<SimCreature>> m_pool;
    std::vector<u32> m_availableIndices;
    std::vector<u32> m_activeIndices;
    u32 m_nextId;
    u32 m_activeCount;
    u32 m_peakActiveCount;
    u32 m_totalAcquisitions;
    u32 m_totalReleases;
    u32 m_poolGrowthCount;

    void removeActiveIndex(u32 activeIndex) {
        u32 lastIndex = m_activeIndices.back();
        m_activeIndices[activeIndex] = lastIndex;
        m_activeIndices.pop_back();

        if (activeIndex < m_activeIndices.size()) {
            m_pool[lastIndex]->activeListIndex = activeIndex;
        }
    }
};

namespace TerrainHeightSample {
    inline float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    inline float grad(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    static const int perm[512] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    inline float perlin2D(float x, float y) {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        float u = fade(x);
        float v = fade(y);

        int A = perm[X] + Y;
        int B = perm[X + 1] + Y;

        return lerp(
            lerp(grad(perm[A], x, y), grad(perm[B], x - 1, y), u),
            lerp(grad(perm[A + 1], x, y - 1), grad(perm[B + 1], x - 1, y - 1), u),
            v
        );
    }

    inline float octaveNoise(float x, float y, int octaves, float persistence) {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            total += perlin2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return (total / maxValue + 1.0f) * 0.5f;
    }

    inline float smoothstep(float edge0, float edge1, float x) {
        float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    }

    inline float sampleHeight(float worldX, float worldZ) {
        using namespace TerrainRendererConfig;

        float nx = worldX / static_cast<float>(WORLD_SIZE) + 0.5f;
        float nz = worldZ / static_cast<float>(WORLD_SIZE) + 0.5f;

        float dx = nx - 0.5f;
        float dz = nz - 0.5f;
        float distance = std::sqrt(dx * dx + dz * dz) * 2.0f;

        float continental = octaveNoise(nx * 2.0f, nz * 2.0f, 4, 0.6f);
        float mountains = octaveNoise(nx * 4.0f + 100.0f, nz * 4.0f + 100.0f, 6, 0.5f);
        mountains = std::pow(mountains, 1.5f);
        float hills = octaveNoise(nx * 8.0f + 50.0f, nz * 8.0f + 50.0f, 4, 0.5f);

        float ridgeNoise = octaveNoise(nx * 3.0f + 200.0f, nz * 3.0f + 200.0f, 4, 0.5f);
        float ridges = 1.0f - std::abs(ridgeNoise * 2.0f - 1.0f);
        ridges = std::pow(ridges, 2.0f) * 0.3f;

        float height = continental * 0.3f + mountains * 0.45f + hills * 0.15f + ridges;

        if (height < 0.35f) {
            height = height * 0.8f;
        } else if (height > 0.7f) {
            float excess = (height - 0.7f) / 0.3f;
            height = 0.7f + excess * excess * 0.3f;
        }

        float islandFactor = 1.0f - smoothstep(0.4f, 0.95f, distance);
        height = height * islandFactor;
        height = height * 1.1f - 0.05f;

        height = std::max(0.0f, std::min(1.0f, height));
        return height * HEIGHT_SCALE;
    }
}

// ============================================================================
// Simulation World - Manages all creatures and food using object pooling
// ============================================================================
class SimulationWorld {
public:
    // Terrain seed for deterministic terrain generation
    uint32_t terrainSeed = 42;

    // Generation counter - tracks the highest generation across all creatures
    uint32_t maxGeneration = 1;

    // Static helper to get terrain height at a world position.
    // Uses the same noise stack as TerrainRendererDX12 to keep creatures on the visible terrain.
    static float GetTerrainHeight(float x, float z) {
        return TerrainSampler::SampleHeight(x, z);
    }

    static float GetSurfaceHeight(const SimCreature& creature) {
        float terrainHeight = GetTerrainHeight(creature.position.x, creature.position.z);

        if (isFlying(creature.type)) {
            float altitude = std::max(10.0f, creature.genome.preferredAltitude);
            return terrainHeight + altitude;
        }

        if (isAquatic(creature.type)) {
            if (TerrainSampler::IsWater(creature.position.x, creature.position.z)) {
                // Use species-specific depth ranges from CreatureType.h
                float waterHeight = TerrainSampler::GetWaterHeight();
                float waterDepth = waterHeight - terrainHeight;

                // Calculate depth using creature's preferred depth and species range
                float minDepth, maxDepth;
                getAquaticSpawnDepthRange(creature.type, minDepth, maxDepth);

                // Scale genome preferredDepth (0.1-0.5) to species depth range
                float normalizedPref = std::clamp(creature.genome.preferredDepth, 0.1f, 0.5f);
                float depthFactor = (normalizedPref - 0.1f) / 0.4f;  // Normalize to 0-1

                float targetDepth = minDepth + depthFactor * (maxDepth - minDepth);
                // Clamp to available water (leave buffer from floor and surface)
                targetDepth = std::clamp(targetDepth, 0.5f, std::max(0.5f, waterDepth - 0.5f));

                return waterHeight - targetDepth;
            }
            // Aquatic creature not in water - return terrain height (will die)
            return terrainHeight + 0.2f;
        }

        return terrainHeight;
    }

    static void UpdateFacing(SimCreature* creature) {
        if (!creature) return;

        glm::vec3 dir = creature->velocity;
        bool allowVertical = isFlying(creature->type) || isAquatic(creature->type);
        if (!allowVertical) {
            dir.y = 0.0f;
        }

        float speed = glm::length(dir);
        if (speed < 0.05f) {
            if (glm::length(creature->facing) < 0.01f) {
                creature->facing = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            return;
        }

        glm::vec3 target = dir / speed;
        if (glm::length(creature->facing) < 0.01f) {
            creature->facing = target;
        } else {
            creature->facing = glm::normalize(glm::mix(creature->facing, target, 0.2f));
        }
    }

    // Creature pool for efficient memory management
    CreaturePool creaturePool;

    // Legacy creature storage (for backward compatibility during transition)
    // New code should use creaturePool instead
    std::vector<std::unique_ptr<SimCreature>> creatures;

    // Active pooled creatures (updated each frame)
    std::vector<SimCreature*> activeCreatures;

    std::vector<std::unique_ptr<Food>> foods;

    u32 nextCreatureId = 0;
    float simulationTime = 0.0f;
    bool paused = false;
    float timeScale = 1.0f;
    float worldBounds = 200.0f;

    // Pool usage mode
    bool usePooling = true;

    // Statistics
    u32 herbivoreCount = 0;
    u32 carnivoreCount = 0;
    u32 totalBirths = 0;
    u32 totalDeaths = 0;
    std::mt19937 respawnRng;

    SimulationWorld() : creaturePool(2048) {
        std::random_device rd;
        respawnRng.seed(rd());
    }

    void SetWorldBounds(float bounds) {
        worldBounds = std::max(10.0f, bounds);
    }

    float GetWorldBounds() const {
        return worldBounds;
    }

    void Initialize(u32 herbivorePopulation, u32 carnivorePopulation, u32 foodCount) {
        std::random_device rd;
        std::mt19937 gen(rd());
        float spawnBound = std::max(1.0f, worldBounds);
        std::uniform_real_distribution<float> posDist(-spawnBound, spawnBound);
        std::uniform_real_distribution<float> energyDist(80.0f, 120.0f);

        auto getSpawnHeight = [](float x, float z) -> float {
            return GetTerrainHeight(x, z);
        };

        const std::array<CreatureType, 3> flyingTypes = {
            CreatureType::FLYING_BIRD,
            CreatureType::FLYING_INSECT,
            CreatureType::AERIAL_PREDATOR
        };
        const std::array<CreatureType, 3> aquaticTypes = {
            CreatureType::AQUATIC_HERBIVORE,
            CreatureType::AQUATIC_PREDATOR,
            CreatureType::AQUATIC_APEX
        };
        std::uniform_int_distribution<size_t> flyingTypeDist(0, flyingTypes.size() - 1);
        std::uniform_int_distribution<size_t> aquaticTypeDist(0, aquaticTypes.size() - 1);

        const int maxAttempts = 20;
        auto spawnCreature = [&](CreatureType type, float energy, bool requireWater) -> bool {
            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                float x = posDist(gen);
                float z = posDist(gen);
                if (requireWater && !TerrainSampler::IsWater(x, z)) {
                    continue;
                }
                float y = getSpawnHeight(x, z);
                glm::vec3 pos(x, y, z);

                if (usePooling) {
                    SimCreature* creature = creaturePool.acquire(pos, type, energy);
                    if (!creature) {
                        std::cerr << "Warning: Failed to acquire creature from pool" << std::endl;
                        return false;
                    }
                    creature->position.y = GetSurfaceHeight(*creature);
                } else {
                    auto creature = std::make_unique<SimCreature>(pos, type, nextCreatureId++);
                    creature->energy = energy;
                    creature->position.y = GetSurfaceHeight(*creature);
                    creatures.push_back(std::move(creature));
                }
                return true;
            }
            return false;
        };

        // Spawn land herbivores
        for (u32 i = 0; i < herbivorePopulation; ++i) {
            if (!spawnCreature(CreatureType::HERBIVORE, energyDist(gen), false)) {
                break;
            }
        }

        // Spawn land carnivores
        for (u32 i = 0; i < carnivorePopulation; ++i) {
            if (!spawnCreature(CreatureType::CARNIVORE, energyDist(gen), false)) {
                break;
            }
        }

        // Ensure some airborne and aquatic life exist at start
        u32 flyingPopulation = std::max(20u, herbivorePopulation / 20);
        u32 aquaticPopulation = std::max(30u, herbivorePopulation / 15);

        for (u32 i = 0; i < flyingPopulation; ++i) {
            CreatureType type = flyingTypes[flyingTypeDist(gen)];
            if (!spawnCreature(type, energyDist(gen), false)) {
                break;
            }
        }

        for (u32 i = 0; i < aquaticPopulation; ++i) {
            CreatureType type = aquaticTypes[aquaticTypeDist(gen)];
            if (!spawnCreature(type, energyDist(gen), true)) {
                break;
            }
        }

        if (usePooling) {
            // Update active creatures list
            activeCreatures = creaturePool.getActiveCreatures();
        }

        // Create food on terrain surface
        const int maxFoodAttempts = 10;
        for (u32 i = 0; i < foodCount; ++i) {
            bool spawned = false;
            for (int attempt = 0; attempt < maxFoodAttempts; ++attempt) {
                float x = posDist(gen);
                float z = posDist(gen);
                if (TerrainSampler::IsWater(x, z)) {
                    continue;
                }
                float y = getSpawnHeight(x, z);
                glm::vec3 pos(x, y, z);
                foods.push_back(std::make_unique<Food>(pos, 50.0f));
                spawned = true;
                break;
            }
            if (!spawned) {
                break;
            }
        }

        UpdateStats();
    }

    // Spawn a new creature using the pool
    // Agent 30: Added population limit check and input validation
    SimCreature* SpawnCreature(const glm::vec3& position, CreatureType type, float energy = 100.0f) {
        // Population limit check (Agent 30 - Stability)
        if (GetAliveCount() >= MAX_CREATURES) {
            return nullptr;  // At capacity, reject spawn
        }

        // Input validation (Agent 30 - Stability)
        glm::vec3 validPos = position;
        if (!std::isfinite(position.x) || !std::isfinite(position.z)) {
            validPos = glm::vec3(0.0f);  // Default to origin if invalid
        }
        validPos.x = std::clamp(validPos.x, -worldBounds, worldBounds);
        validPos.z = std::clamp(validPos.z, -worldBounds, worldBounds);

        float validEnergy = std::isfinite(energy) ? std::max(1.0f, energy) : 100.0f;

        if (usePooling) {
            SimCreature* creature = creaturePool.acquire(validPos, type, validEnergy);
            if (creature) {
                creature->position.y = GetSurfaceHeight(*creature);
                totalBirths++;
            }
            return creature;
        } else {
            auto creature = std::make_unique<SimCreature>(validPos, type, nextCreatureId++);
            creature->energy = validEnergy;
            creature->position.y = GetSurfaceHeight(*creature);
            SimCreature* ptr = creature.get();
            creatures.push_back(std::move(creature));
            totalBirths++;
            return ptr;
        }
    }

    void SpawnCreatures(CreatureType type, u32 count, float radius, float energyMin, float energyMax) {
        if (count == 0) return;

        float spawnRadius = std::max(1.0f, std::min(radius, worldBounds));
        float minEnergy = std::min(energyMin, energyMax);
        float maxEnergy = std::max(energyMin, energyMax);

        std::uniform_real_distribution<float> posDist(-spawnRadius, spawnRadius);
        std::uniform_real_distribution<float> energyDist(minEnergy, maxEnergy);

        const bool requireWater = isAquatic(type);
        const int maxAttempts = 10;

        for (u32 i = 0; i < count; ++i) {
            bool spawned = false;
            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                float x = posDist(respawnRng);
                float z = posDist(respawnRng);
                if (requireWater && !TerrainSampler::IsWater(x, z)) {
                    continue;
                }
                float y = GetTerrainHeight(x, z);
                glm::vec3 pos(x, y, z);
                float energy = energyDist(respawnRng);

                if (SpawnCreature(pos, type, energy)) {
                    spawned = true;
                    break;
                }
            }

            if (!spawned) {
                break;
            }
        }

        if (usePooling) {
            activeCreatures = creaturePool.getActiveCreatures();
        }
        UpdateStats();
    }

    void SpawnFood(u32 count, float radius, float amountMin, float amountMax) {
        if (count == 0) return;

        float spawnRadius = std::max(1.0f, std::min(radius, worldBounds));
        float minAmount = std::min(amountMin, amountMax);
        float maxAmount = std::max(amountMin, amountMax);

        std::uniform_real_distribution<float> posDist(-spawnRadius, spawnRadius);
        std::uniform_real_distribution<float> amountDist(minAmount, maxAmount);

        const int maxAttempts = 10;

        for (u32 i = 0; i < count && foods.size() < MAX_FOOD_SOURCES; ++i) {
            bool spawned = false;
            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                float x = posDist(respawnRng);
                float z = posDist(respawnRng);
                if (TerrainSampler::IsWater(x, z)) {
                    continue;
                }
                float y = GetTerrainHeight(x, z);
                glm::vec3 pos(x, y, z);
                foods.push_back(std::make_unique<Food>(pos, amountDist(respawnRng)));
                spawned = true;
                break;
            }

            if (!spawned) {
                break;
            }
        }

        UpdateStats();
    }

    void Reset(u32 herbivorePopulation, u32 carnivorePopulation, u32 foodCount) {
        if (usePooling) {
            creaturePool.clear();
            activeCreatures.clear();
        }
        creatures.clear();
        foods.clear();
        nextCreatureId = 0;
        maxGeneration = 1;
        totalBirths = 0;
        totalDeaths = 0;
        simulationTime = 0.0f;

        Initialize(herbivorePopulation, carnivorePopulation, foodCount);
    }

    // Get RNG state as string for serialization
    std::string getRngState() const {
        std::ostringstream oss;
        oss << respawnRng;
        return oss.str();
    }

    // Set RNG state from string for deserialization
    void setRngState(const std::string& state) {
        if (!state.empty()) {
            std::istringstream iss(state);
            iss >> respawnRng;
        }
    }

    void Update(float dt, const std::vector<SteeringOutput>* gpuSteeringResults = nullptr) {
        if (paused) return;

        float scaledDt = dt * timeScale;
        simulationTime += scaledDt;

        if (usePooling) {
            // Update active creatures list from pool
            activeCreatures = creaturePool.getActiveCreatures();

            // Apply GPU steering results if available
            if (gpuSteeringResults && gpuSteeringResults->size() >= activeCreatures.size()) {
                for (size_t i = 0; i < activeCreatures.size(); ++i) {
                    SimCreature* creature = activeCreatures[i];
                    if (!creature->alive) continue;

                    const auto& steering = (*gpuSteeringResults)[i];
                    glm::vec3 force(steering.steeringForce.x, steering.steeringForce.y, steering.steeringForce.z);

                    // Apply steering force
                    creature->velocity += force * scaledDt;

                    // Limit speed
                    float speed = glm::length(creature->velocity);
                    if (speed > 10.0f) {
                        creature->velocity = glm::normalize(creature->velocity) * 10.0f;
                    }

                    // Update position
                    creature->position += creature->velocity * scaledDt;

                    // Snap Y position to surface height (ground, water, or flight altitude)
                    creature->position.y = GetSurfaceHeight(*creature);
                    UpdateFacing(creature);

                    // Consume energy
                    creature->energy -= 0.1f * scaledDt;
                    if (creature->energy <= 0.0f) {
                        creature->alive = false;
                        totalDeaths++;
                    }
                }
            } else {
                // ================================================================
                // CPU Fallback - Proper Steering Behaviors (Pooled Mode)
                // ================================================================
                // Implements food seeking, predator avoidance, prey hunting, flocking

                // Steering constants
                constexpr float FOOD_SEEK_RANGE_SQ = 3600.0f;      // 60^2
                constexpr float PREDATOR_AVOID_RANGE_SQ = 1600.0f; // 40^2
                constexpr float PREY_HUNT_RANGE_SQ = 2500.0f;      // 50^2
                constexpr float FLOCK_RANGE_SQ = 900.0f;           // 30^2
                constexpr float FOOD_SEEK_WEIGHT = 2.0f;
                constexpr float PREDATOR_AVOID_WEIGHT = 4.0f;
                constexpr float PREY_HUNT_WEIGHT = 2.5f;
                constexpr float FLOCK_WEIGHT = 0.5f;
                constexpr float WANDER_WEIGHT = 0.3f;
                constexpr float MAX_HERBIVORE_SPEED = 8.0f;
                constexpr float MAX_CARNIVORE_SPEED = 10.0f;
                constexpr float MAX_FORCE = 15.0f;

                // Single random generator (static to avoid recreation)
                static std::mt19937 cpuGen(42);
                std::uniform_real_distribution<float> wanderDist(-1.0f, 1.0f);

                // Pre-collect alive creatures by type
                std::vector<SimCreature*> aliveHerbivores;
                std::vector<SimCreature*> aliveCarnivores;
                aliveHerbivores.reserve(activeCreatures.size());
                aliveCarnivores.reserve(activeCreatures.size() / 4);

                for (SimCreature* c : activeCreatures) {
                    if (!c->alive) continue;
                    if (c->type == CreatureType::HERBIVORE || c->type == CreatureType::GRAZER) {
                        aliveHerbivores.push_back(c);
                    } else {
                        aliveCarnivores.push_back(c);
                    }
                }

                // Process each creature
                for (SimCreature* creature : activeCreatures) {
                    if (!creature->alive) continue;

                    glm::vec3 steeringForce(0.0f);
                    bool hasTarget = false;
                    const glm::vec3& pos = creature->position;
                    const bool isHerbivore = (creature->type == CreatureType::HERBIVORE ||
                                              creature->type == CreatureType::GRAZER);

                    if (isHerbivore) {
                        // 1. Predator Avoidance (highest priority)
                        glm::vec3 avoidForce(0.0f);
                        int nearbyPredators = 0;

                        for (SimCreature* pred : aliveCarnivores) {
                            float dx = pred->position.x - pos.x;
                            float dz = pred->position.z - pos.z;
                            float distSq = dx * dx + dz * dz;

                            if (distSq < PREDATOR_AVOID_RANGE_SQ && distSq > 0.01f) {
                                float invDist = 1.0f / std::sqrt(distSq);
                                glm::vec3 fleeDir(-dx * invDist, 0.0f, -dz * invDist);
                                float urgency = 1.0f - (distSq / PREDATOR_AVOID_RANGE_SQ);
                                avoidForce += fleeDir * urgency;
                                nearbyPredators++;
                                creature->fear = std::max(creature->fear, urgency);
                            }
                        }

                        if (nearbyPredators > 0) {
                            float len = glm::length(avoidForce);
                            if (len > 0.01f) avoidForce = (avoidForce / len) * PREDATOR_AVOID_WEIGHT;
                            steeringForce += avoidForce;
                            hasTarget = true;
                        } else {
                            creature->fear *= 0.95f;
                        }

                        // 2. Food Seeking (when not fleeing and hungry)
                        if (nearbyPredators == 0 && creature->energy < 80.0f) {
                            glm::vec3 nearestFoodDir(0.0f);
                            float nearestDistSq = FOOD_SEEK_RANGE_SQ;
                            bool foundFood = false;

                            for (const auto& food : foods) {
                                if (food->amount <= 0.0f) continue;
                                float dx = food->position.x - pos.x;
                                float dz = food->position.z - pos.z;
                                float distSq = dx * dx + dz * dz;

                                if (distSq < nearestDistSq && distSq > 0.01f) {
                                    nearestDistSq = distSq;
                                    float invDist = 1.0f / std::sqrt(distSq);
                                    nearestFoodDir = glm::vec3(dx * invDist, 0.0f, dz * invDist);
                                    foundFood = true;
                                }
                            }

                            if (foundFood) {
                                float hungerBonus = 1.0f + (80.0f - creature->energy) / 80.0f;
                                steeringForce += nearestFoodDir * FOOD_SEEK_WEIGHT * hungerBonus;
                                hasTarget = true;
                            }
                        }

                        // 3. Flocking with herbivores
                        glm::vec3 cohesionCenter(0.0f);
                        int flockmates = 0;

                        for (SimCreature* other : aliveHerbivores) {
                            if (other == creature) continue;
                            float dx = other->position.x - pos.x;
                            float dz = other->position.z - pos.z;
                            float distSq = dx * dx + dz * dz;

                            if (distSq < FLOCK_RANGE_SQ && distSq > 1.0f) {
                                cohesionCenter += other->position;
                                flockmates++;
                            }
                        }

                        if (flockmates > 0) {
                            cohesionCenter /= static_cast<float>(flockmates);
                            glm::vec3 toCenter = cohesionCenter - pos;
                            float dist = glm::length(toCenter);
                            if (dist > 0.1f) {
                                steeringForce += (toCenter / dist) * FLOCK_WEIGHT;
                            }
                        }

                    } else {
                        // CARNIVORE behaviors

                        // 1. Prey Hunting
                        glm::vec3 nearestPreyDir(0.0f);
                        float nearestDistSq = PREY_HUNT_RANGE_SQ;
                        bool foundPrey = false;

                        for (SimCreature* prey : aliveHerbivores) {
                            float dx = prey->position.x - pos.x;
                            float dz = prey->position.z - pos.z;
                            float distSq = dx * dx + dz * dz;

                            if (distSq < nearestDistSq && distSq > 0.01f) {
                                nearestDistSq = distSq;
                                float invDist = 1.0f / std::sqrt(distSq);
                                nearestPreyDir = glm::vec3(dx * invDist, 0.0f, dz * invDist);
                                foundPrey = true;
                            }
                        }

                        if (foundPrey) {
                            float hungerBonus = 1.0f + (100.0f - creature->energy) / 100.0f;
                            steeringForce += nearestPreyDir * PREY_HUNT_WEIGHT * hungerBonus;
                            hasTarget = true;
                        }

                        // 2. Pack flocking (when not hunting)
                        if (!foundPrey) {
                            glm::vec3 packCenter(0.0f);
                            int packmates = 0;

                            for (SimCreature* other : aliveCarnivores) {
                                if (other == creature) continue;
                                float dx = other->position.x - pos.x;
                                float dz = other->position.z - pos.z;
                                float distSq = dx * dx + dz * dz;

                                if (distSq < FLOCK_RANGE_SQ && distSq > 1.0f) {
                                    packCenter += other->position;
                                    packmates++;
                                }
                            }

                            if (packmates > 0) {
                                packCenter /= static_cast<float>(packmates);
                                glm::vec3 toCenter = packCenter - pos;
                                float dist = glm::length(toCenter);
                                if (dist > 0.1f) {
                                    steeringForce += (toCenter / dist) * FLOCK_WEIGHT * 0.5f;
                                }
                            }
                        }
                    }

                    // Random wander when no target
                    if (!hasTarget) {
                        glm::vec3 wander(wanderDist(cpuGen), 0.0f, wanderDist(cpuGen));
                        steeringForce += wander * WANDER_WEIGHT;
                    }

                    // Clamp steering force
                    float forceLen = glm::length(steeringForce);
                    if (forceLen > MAX_FORCE) {
                        steeringForce = (steeringForce / forceLen) * MAX_FORCE;
                    }

                    // Apply force
                    creature->velocity += steeringForce * scaledDt;

                    // Limit speed
                    float maxSpeed = isHerbivore ? MAX_HERBIVORE_SPEED : MAX_CARNIVORE_SPEED;
                    float speed = glm::length(creature->velocity);
                    if (speed > maxSpeed) {
                        creature->velocity = (creature->velocity / speed) * maxSpeed;
                    }

                    // Update position
                    creature->position += creature->velocity * scaledDt;

                    // Snap Y position to surface height (ground, water, or flight altitude)
                    creature->position.y = GetSurfaceHeight(*creature);
                    UpdateFacing(creature);

                    // World bounds
                    const float bound = worldBounds;
                    const float pushZone = std::max(20.0f, bound * 0.1f);
                    const float pushStrength = 5.0f;

                    if (creature->position.x > bound - pushZone)
                        creature->velocity.x -= pushStrength * scaledDt;
                    else if (creature->position.x < -bound + pushZone)
                        creature->velocity.x += pushStrength * scaledDt;
                    if (creature->position.z > bound - pushZone)
                        creature->velocity.z -= pushStrength * scaledDt;
                    else if (creature->position.z < -bound + pushZone)
                        creature->velocity.z += pushStrength * scaledDt;

                    // Hard bounds clamp (Agent 30 - Stability fix)
                    creature->position.x = std::clamp(creature->position.x, -bound, bound);
                    creature->position.z = std::clamp(creature->position.z, -bound, bound);

                    // NaN/Inf check (Agent 30 - Stability fix)
                    if (!std::isfinite(creature->position.x) || !std::isfinite(creature->position.z)) {
                        creature->position = glm::vec3(0.0f, GetSurfaceHeight(*creature), 0.0f);
                        creature->velocity = glm::vec3(0.0f);
                    }
                    if (!std::isfinite(creature->velocity.x) || !std::isfinite(creature->velocity.z)) {
                        creature->velocity = glm::vec3(0.0f);
                    }

                    // Energy consumption
                    float movementCost = 0.1f + speed * 0.01f;
                    creature->energy -= movementCost * scaledDt;

                    if (creature->energy <= 0.0f) {
                        creature->alive = false;
                        totalDeaths++;
                    }
                }

                // Food consumption for herbivores
                constexpr float FOOD_EAT_RANGE_SQ = 4.0f;
                constexpr float MAX_ENERGY = 150.0f;

                for (SimCreature* herb : aliveHerbivores) {
                    if (!herb->alive) continue;
                    for (auto& food : foods) {
                        if (food->amount <= 0.0f) continue;
                        float dx = food->position.x - herb->position.x;
                        float dz = food->position.z - herb->position.z;
                        float distSq = dx * dx + dz * dz;

                        if (distSq < FOOD_EAT_RANGE_SQ) {
                            float eatAmount = std::min(food->amount, 10.0f * scaledDt);
                            herb->energy = std::min(herb->energy + eatAmount, MAX_ENERGY);
                            food->amount -= eatAmount;
                            break;
                        }
                    }
                }

                // Hunting kills for carnivores
                constexpr float KILL_RANGE_SQ = 9.0f;

                for (SimCreature* carn : aliveCarnivores) {
                    if (!carn->alive) continue;
                    for (SimCreature* prey : aliveHerbivores) {
                        if (!prey->alive) continue;
                        float dx = prey->position.x - carn->position.x;
                        float dz = prey->position.z - carn->position.z;
                        float distSq = dx * dx + dz * dz;

                        if (distSq < KILL_RANGE_SQ) {
                            prey->alive = false;
                            totalDeaths++;
                            carn->energy = std::min(carn->energy + 50.0f, MAX_ENERGY);
                            break;
                        }
                    }
                }
            }

            // Release dead creatures back to pool
            creaturePool.releaseDeadCreatures();

        } else {
            // Legacy mode: direct allocation
            // Apply GPU steering results if available
            if (gpuSteeringResults && gpuSteeringResults->size() >= creatures.size()) {
                for (size_t i = 0; i < creatures.size(); ++i) {
                    if (!creatures[i]->alive) continue;

                    const auto& steering = (*gpuSteeringResults)[i];
                    glm::vec3 force(steering.steeringForce.x, steering.steeringForce.y, steering.steeringForce.z);

                    creatures[i]->velocity += force * scaledDt;

                    float speed = glm::length(creatures[i]->velocity);
                    if (speed > 10.0f) {
                        creatures[i]->velocity = glm::normalize(creatures[i]->velocity) * 10.0f;
                    }

                    creatures[i]->position += creatures[i]->velocity * scaledDt;
                    creatures[i]->energy -= 0.1f * scaledDt;

                    if (creatures[i]->energy <= 0.0f) {
                        creatures[i]->alive = false;
                        totalDeaths++;
                    }
                }
            } else {
                // ================================================================
                // CPU Fallback - Proper Steering Behaviors (Legacy Mode)
                // ================================================================
                // Same behaviors as pooled mode but using unique_ptr vector

                // Steering constants
                constexpr float FOOD_SEEK_RANGE_SQ = 3600.0f;      // 60^2
                constexpr float PREDATOR_AVOID_RANGE_SQ = 1600.0f; // 40^2
                constexpr float PREY_HUNT_RANGE_SQ = 2500.0f;      // 50^2
                constexpr float FLOCK_RANGE_SQ = 900.0f;           // 30^2
                constexpr float FOOD_SEEK_WEIGHT = 2.0f;
                constexpr float PREDATOR_AVOID_WEIGHT = 4.0f;
                constexpr float PREY_HUNT_WEIGHT = 2.5f;
                constexpr float FLOCK_WEIGHT = 0.5f;
                constexpr float WANDER_WEIGHT = 0.3f;
                constexpr float MAX_HERBIVORE_SPEED = 8.0f;
                constexpr float MAX_CARNIVORE_SPEED = 10.0f;
                constexpr float MAX_FORCE = 15.0f;

                // Single random generator
                static std::mt19937 cpuGenLegacy(43);
                std::uniform_real_distribution<float> wanderDist(-1.0f, 1.0f);

                // Pre-collect alive creature indices by type
                std::vector<size_t> aliveHerbivoreIdx;
                std::vector<size_t> aliveCarnivoreIdx;
                aliveHerbivoreIdx.reserve(creatures.size());
                aliveCarnivoreIdx.reserve(creatures.size() / 4);

                for (size_t i = 0; i < creatures.size(); ++i) {
                    if (!creatures[i]->alive) continue;
                    if (creatures[i]->type == CreatureType::HERBIVORE ||
                        creatures[i]->type == CreatureType::GRAZER) {
                        aliveHerbivoreIdx.push_back(i);
                    } else {
                        aliveCarnivoreIdx.push_back(i);
                    }
                }

                // Process each creature
                for (size_t i = 0; i < creatures.size(); ++i) {
                    auto& creature = creatures[i];
                    if (!creature->alive) continue;

                    glm::vec3 steeringForce(0.0f);
                    bool hasTarget = false;
                    const glm::vec3& pos = creature->position;
                    const bool isHerbivore = (creature->type == CreatureType::HERBIVORE ||
                                              creature->type == CreatureType::GRAZER);

                    if (isHerbivore) {
                        // 1. Predator Avoidance
                        glm::vec3 avoidForce(0.0f);
                        int nearbyPredators = 0;

                        for (size_t ci : aliveCarnivoreIdx) {
                            const glm::vec3& predPos = creatures[ci]->position;
                            float dx = predPos.x - pos.x;
                            float dz = predPos.z - pos.z;
                            float distSq = dx * dx + dz * dz;

                            if (distSq < PREDATOR_AVOID_RANGE_SQ && distSq > 0.01f) {
                                float invDist = 1.0f / std::sqrt(distSq);
                                glm::vec3 fleeDir(-dx * invDist, 0.0f, -dz * invDist);
                                float urgency = 1.0f - (distSq / PREDATOR_AVOID_RANGE_SQ);
                                avoidForce += fleeDir * urgency;
                                nearbyPredators++;
                                creature->fear = std::max(creature->fear, urgency);
                            }
                        }

                        if (nearbyPredators > 0) {
                            float len = glm::length(avoidForce);
                            if (len > 0.01f) avoidForce = (avoidForce / len) * PREDATOR_AVOID_WEIGHT;
                            steeringForce += avoidForce;
                            hasTarget = true;
                        } else {
                            creature->fear *= 0.95f;
                        }

                        // 2. Food Seeking
                        if (nearbyPredators == 0 && creature->energy < 80.0f) {
                            glm::vec3 nearestFoodDir(0.0f);
                            float nearestDistSq = FOOD_SEEK_RANGE_SQ;
                            bool foundFood = false;

                            for (const auto& food : foods) {
                                if (food->amount <= 0.0f) continue;
                                float dx = food->position.x - pos.x;
                                float dz = food->position.z - pos.z;
                                float distSq = dx * dx + dz * dz;

                                if (distSq < nearestDistSq && distSq > 0.01f) {
                                    nearestDistSq = distSq;
                                    float invDist = 1.0f / std::sqrt(distSq);
                                    nearestFoodDir = glm::vec3(dx * invDist, 0.0f, dz * invDist);
                                    foundFood = true;
                                }
                            }

                            if (foundFood) {
                                float hungerBonus = 1.0f + (80.0f - creature->energy) / 80.0f;
                                steeringForce += nearestFoodDir * FOOD_SEEK_WEIGHT * hungerBonus;
                                hasTarget = true;
                            }
                        }

                        // 3. Flocking
                        glm::vec3 cohesionCenter(0.0f);
                        int flockmates = 0;

                        for (size_t hi : aliveHerbivoreIdx) {
                            if (hi == i) continue;
                            const glm::vec3& otherPos = creatures[hi]->position;
                            float dx = otherPos.x - pos.x;
                            float dz = otherPos.z - pos.z;
                            float distSq = dx * dx + dz * dz;

                            if (distSq < FLOCK_RANGE_SQ && distSq > 1.0f) {
                                cohesionCenter += otherPos;
                                flockmates++;
                            }
                        }

                        if (flockmates > 0) {
                            cohesionCenter /= static_cast<float>(flockmates);
                            glm::vec3 toCenter = cohesionCenter - pos;
                            float dist = glm::length(toCenter);
                            if (dist > 0.1f) {
                                steeringForce += (toCenter / dist) * FLOCK_WEIGHT;
                            }
                        }

                    } else {
                        // CARNIVORE behaviors

                        // 1. Prey Hunting
                        glm::vec3 nearestPreyDir(0.0f);
                        float nearestDistSq = PREY_HUNT_RANGE_SQ;
                        bool foundPrey = false;

                        for (size_t hi : aliveHerbivoreIdx) {
                            const glm::vec3& preyPos = creatures[hi]->position;
                            float dx = preyPos.x - pos.x;
                            float dz = preyPos.z - pos.z;
                            float distSq = dx * dx + dz * dz;

                            if (distSq < nearestDistSq && distSq > 0.01f) {
                                nearestDistSq = distSq;
                                float invDist = 1.0f / std::sqrt(distSq);
                                nearestPreyDir = glm::vec3(dx * invDist, 0.0f, dz * invDist);
                                foundPrey = true;
                            }
                        }

                        if (foundPrey) {
                            float hungerBonus = 1.0f + (100.0f - creature->energy) / 100.0f;
                            steeringForce += nearestPreyDir * PREY_HUNT_WEIGHT * hungerBonus;
                            hasTarget = true;
                        }

                        // 2. Pack flocking
                        if (!foundPrey) {
                            glm::vec3 packCenter(0.0f);
                            int packmates = 0;

                            for (size_t ci : aliveCarnivoreIdx) {
                                if (ci == i) continue;
                                const glm::vec3& otherPos = creatures[ci]->position;
                                float dx = otherPos.x - pos.x;
                                float dz = otherPos.z - pos.z;
                                float distSq = dx * dx + dz * dz;

                                if (distSq < FLOCK_RANGE_SQ && distSq > 1.0f) {
                                    packCenter += otherPos;
                                    packmates++;
                                }
                            }

                            if (packmates > 0) {
                                packCenter /= static_cast<float>(packmates);
                                glm::vec3 toCenter = packCenter - pos;
                                float dist = glm::length(toCenter);
                                if (dist > 0.1f) {
                                    steeringForce += (toCenter / dist) * FLOCK_WEIGHT * 0.5f;
                                }
                            }
                        }
                    }

                    // Random wander
                    if (!hasTarget) {
                        glm::vec3 wander(wanderDist(cpuGenLegacy), 0.0f, wanderDist(cpuGenLegacy));
                        steeringForce += wander * WANDER_WEIGHT;
                    }

                    // Clamp force
                    float forceLen = glm::length(steeringForce);
                    if (forceLen > MAX_FORCE) {
                        steeringForce = (steeringForce / forceLen) * MAX_FORCE;
                    }

                    // Apply force
                    creature->velocity += steeringForce * scaledDt;

                    // Limit speed
                    float maxSpeed = isHerbivore ? MAX_HERBIVORE_SPEED : MAX_CARNIVORE_SPEED;
                    float speed = glm::length(creature->velocity);
                    if (speed > maxSpeed) {
                        creature->velocity = (creature->velocity / speed) * maxSpeed;
                    }

                    // Update position
                    creature->position += creature->velocity * scaledDt;

                    // Snap Y position to surface height (ground, water, or flight altitude)
                    creature->position.y = GetSurfaceHeight(*creature);
                    UpdateFacing(creature.get());

                    // World bounds
                    const float bound = worldBounds;
                    const float pushZone = std::max(20.0f, bound * 0.1f);
                    const float pushStrength = 5.0f;

                    if (creature->position.x > bound - pushZone)
                        creature->velocity.x -= pushStrength * scaledDt;
                    else if (creature->position.x < -bound + pushZone)
                        creature->velocity.x += pushStrength * scaledDt;
                    if (creature->position.z > bound - pushZone)
                        creature->velocity.z -= pushStrength * scaledDt;
                    else if (creature->position.z < -bound + pushZone)
                        creature->velocity.z += pushStrength * scaledDt;

                    // Hard bounds clamp (Agent 30 - Stability fix)
                    creature->position.x = std::clamp(creature->position.x, -bound, bound);
                    creature->position.z = std::clamp(creature->position.z, -bound, bound);

                    // NaN/Inf check (Agent 30 - Stability fix)
                    if (!std::isfinite(creature->position.x) || !std::isfinite(creature->position.z)) {
                        creature->position = glm::vec3(0.0f, GetSurfaceHeight(*creature), 0.0f);
                        creature->velocity = glm::vec3(0.0f);
                    }
                    if (!std::isfinite(creature->velocity.x) || !std::isfinite(creature->velocity.z)) {
                        creature->velocity = glm::vec3(0.0f);
                    }

                    // Energy consumption
                    float movementCost = 0.1f + speed * 0.01f;
                    creature->energy -= movementCost * scaledDt;

                    if (creature->energy <= 0.0f) {
                        creature->alive = false;
                        totalDeaths++;
                    }
                }

                // Food consumption
                constexpr float FOOD_EAT_RANGE_SQ = 4.0f;
                constexpr float MAX_ENERGY = 150.0f;

                for (size_t hi : aliveHerbivoreIdx) {
                    auto& herb = creatures[hi];
                    if (!herb->alive) continue;
                    for (auto& food : foods) {
                        if (food->amount <= 0.0f) continue;
                        float dx = food->position.x - herb->position.x;
                        float dz = food->position.z - herb->position.z;
                        float distSq = dx * dx + dz * dz;

                        if (distSq < FOOD_EAT_RANGE_SQ) {
                            float eatAmount = std::min(food->amount, 10.0f * scaledDt);
                            herb->energy = std::min(herb->energy + eatAmount, MAX_ENERGY);
                            food->amount -= eatAmount;
                            break;
                        }
                    }
                }

                // Hunting kills
                constexpr float KILL_RANGE_SQ = 9.0f;

                for (size_t ci : aliveCarnivoreIdx) {
                    auto& carn = creatures[ci];
                    if (!carn->alive) continue;
                    for (size_t hi : aliveHerbivoreIdx) {
                        auto& prey = creatures[hi];
                        if (!prey->alive) continue;
                        float dx = prey->position.x - carn->position.x;
                        float dz = prey->position.z - carn->position.z;
                        float distSq = dx * dx + dz * dz;

                        if (distSq < KILL_RANGE_SQ) {
                            prey->alive = false;
                            totalDeaths++;
                            carn->energy = std::min(carn->energy + 50.0f, MAX_ENERGY);
                            break;
                        }
                    }
                }
            }
        }

        // Respawn food periodically
        std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
        float spawnBound = std::max(1.0f, worldBounds);
        std::uniform_real_distribution<float> posDist(-spawnBound, spawnBound);

        if (spawnChance(respawnRng) < 0.1f * scaledDt && foods.size() < MAX_FOOD_SOURCES) {
            for (int attempt = 0; attempt < 5; ++attempt) {
                float x = posDist(respawnRng);
                float z = posDist(respawnRng);
                if (TerrainSampler::IsWater(x, z)) {
                    continue;
                }
                float y = GetTerrainHeight(x, z);
                glm::vec3 pos(x, y, z);
                foods.push_back(std::make_unique<Food>(pos, 50.0f));
                break;
            }
        }

        UpdateStats();
    }

    void UpdateStats() {
        herbivoreCount = 0;
        carnivoreCount = 0;

        if (usePooling) {
            for (SimCreature* c : activeCreatures) {
                if (!c->alive) continue;
                if (c->type == CreatureType::HERBIVORE || c->type == CreatureType::GRAZER) {
                    herbivoreCount++;
                } else {
                    carnivoreCount++;
                }
            }
        } else {
            for (const auto& c : creatures) {
                if (!c->alive) continue;
                if (c->type == CreatureType::HERBIVORE || c->type == CreatureType::GRAZER) {
                    herbivoreCount++;
                } else {
                    carnivoreCount++;
                }
            }
        }
    }

    u32 GetAliveCount() const {
        if (usePooling) {
            return creaturePool.getActiveCount();
        } else {
            u32 count = 0;
            for (const auto& c : creatures) {
                if (c->alive) count++;
            }
            return count;
        }
    }

    // Get creature count for GPU steering - returns active pooled creatures or legacy creatures
    size_t GetCreatureCountForGPU() const {
        if (usePooling) {
            return activeCreatures.size();
        } else {
            return creatures.size();
        }
    }
};

// ============================================================================
// Performance Profiling (Phase 2 - Frame Timing Breakdown)
// ============================================================================
struct FrameTimings {
    float creatureUpdate = 0.0f;      // Creature behavior and physics
    float gpuSteeringDispatch = 0.0f; // GPU compute shader dispatch
    float gpuSteeringReadback = 0.0f; // GPU to CPU data transfer
    float spatialGridRebuild = 0.0f;  // Spatial partitioning update
    float neuralNetworkEval = 0.0f;   // Neural network forward pass
    float rendering = 0.0f;           // Main render pass
    float postProcessing = 0.0f;      // Post-processing effects
    float uiRendering = 0.0f;         // ImGui rendering
    float replayRecording = 0.0f;     // Replay frame capture
    float total = 0.0f;               // Total frame time

    // History for graphs (last 120 frames = 2 seconds at 60 FPS)
    static constexpr size_t HISTORY_SIZE = 120;
    float fpsHistory[HISTORY_SIZE] = {0};
    float frameTimeHistory[HISTORY_SIZE] = {0};
    float creatureUpdateHistory[HISTORY_SIZE] = {0};
    float gpuComputeHistory[HISTORY_SIZE] = {0};
    float renderingHistory[HISTORY_SIZE] = {0};
    size_t historyIndex = 0;

    void pushHistory() {
        fpsHistory[historyIndex] = 1.0f / std::max(total, 0.001f);
        frameTimeHistory[historyIndex] = total * 1000.0f;
        creatureUpdateHistory[historyIndex] = creatureUpdate * 1000.0f;
        gpuComputeHistory[historyIndex] = (gpuSteeringDispatch + gpuSteeringReadback) * 1000.0f;
        renderingHistory[historyIndex] = rendering * 1000.0f;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }

    void reset() {
        creatureUpdate = 0.0f;
        gpuSteeringDispatch = 0.0f;
        gpuSteeringReadback = 0.0f;
        spatialGridRebuild = 0.0f;
        neuralNetworkEval = 0.0f;
        rendering = 0.0f;
        postProcessing = 0.0f;
        uiRendering = 0.0f;
        replayRecording = 0.0f;
        total = 0.0f;
    }
};

// Helper macro for timing code blocks
#define PROFILE_SCOPE(timingVar) \
    auto _profile_start_##timingVar = std::chrono::high_resolution_clock::now()
#define PROFILE_END(timingVar) \
    timingVar = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - _profile_start_##timingVar).count()

// ============================================================================
// Notification System (Phase 5 - Polish)
// ============================================================================
enum class NotificationType {
    Info,
    Success,
    Warning,
    Error
};

struct Notification {
    std::string text;
    NotificationType type;
    float duration;
    float elapsed;
};

class NotificationManager {
public:
    void show(const std::string& text, NotificationType type = NotificationType::Info, float duration = 3.0f) {
        m_notifications.push_back({text, type, duration, 0.0f});
    }

    void update(float dt) {
        for (auto it = m_notifications.begin(); it != m_notifications.end(); ) {
            it->elapsed += dt;
            if (it->elapsed >= it->duration) {
                it = m_notifications.erase(it);
            } else {
                ++it;
            }
        }
    }

    void render(float screenWidth) {
        float y = 80.0f;  // Start below status message
        int id = 0;

        for (const auto& notif : m_notifications) {
            float alpha = 1.0f - (notif.elapsed / notif.duration);
            alpha = std::min(1.0f, alpha * 2.0f);  // Fade out in last half

            ImGui::SetNextWindowPos(ImVec2(screenWidth - 310, y));
            ImGui::SetNextWindowBgAlpha(alpha * 0.8f);

            char windowId[32];
            snprintf(windowId, sizeof(windowId), "##notif%d", id++);

            ImGui::Begin(windowId, nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoInputs);

            ImVec4 color = getColorForType(notif.type);
            color.w = alpha;
            ImGui::TextColored(color, "%s", notif.text.c_str());

            ImGui::End();
            y += 35.0f;
        }
    }

private:
    std::vector<Notification> m_notifications;

    static ImVec4 getColorForType(NotificationType type) {
        switch (type) {
            case NotificationType::Success: return ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
            case NotificationType::Warning: return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
            case NotificationType::Error:   return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            default:                         return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
};

// ============================================================================
// Camera Transition System (Phase 5 - Smooth Camera)
// ============================================================================
struct MainCameraTransition {
    glm::vec3 startTarget;
    glm::vec3 endTarget;
    float startYaw, endYaw;
    float startPitch, endPitch;
    float startDistance, endDistance;
    float duration;
    float progress;
    bool active;

    MainCameraTransition() : duration(0), progress(0), active(false) {}

    void start(const glm::vec3& target, float yaw, float pitch, float distance, float dur) {
        // Save current state (will be set from AppState before calling)
        endTarget = target;
        endYaw = yaw;
        endPitch = pitch;
        endDistance = distance;
        duration = dur;
        progress = 0.0f;
        active = true;
    }

    void update(float dt) {
        if (!active) return;

        progress += dt / duration;
        if (progress >= 1.0f) {
            progress = 1.0f;
            active = false;
        }
    }

    // Smooth step function for easing
    static float smoothstep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float getInterpolatedValue(float start, float end) const {
        float t = smoothstep(0.0f, 1.0f, progress);
        return start + (end - start) * t;
    }

    glm::vec3 getInterpolatedTarget() const {
        float t = smoothstep(0.0f, 1.0f, progress);
        return startTarget + (endTarget - startTarget) * t;
    }

    // Extended transition with position support
    glm::vec3 startPosition;
    glm::vec3 endPosition;
    bool usePositionTarget = false;

    void startWithPosition(const glm::vec3& pos, const glm::vec3& target, float dur) {
        endPosition = pos;
        endTarget = target;
        duration = dur;
        progress = 0.0f;
        active = true;
        usePositionTarget = true;
    }

    glm::vec3 getInterpolatedPosition() const {
        float t = smoothstep(0.0f, 1.0f, progress);
        return startPosition + (endPosition - startPosition) * t;
    }
};

// Forward declaration of StartCameraTransition helper (defined after g_app)
static float GetCreatureRenderSurfaceHeight(const Creature& creature);
static void ResetUnifiedFoodSources(u32 plantCount);
static void SpawnInitialCreatures(const ui::EvolutionStartPreset& evolutionPreset);
static void UpdateUnifiedSimulation(float dt);
static void StepUnifiedSimulation(int count);
static void StartWorldGeneration(const ui::WorldGenConfig& menuConfig,
                                 const ui::EvolutionStartPreset& evolutionPreset,
                                 bool godModeEnabled);
static void ApplyGeneratedWorldData(const WorldGenConfig& proceduralConfig,
                                    const ui::EvolutionStartPreset& evolutionPreset,
                                    bool godModeEnabled);

// ========================================================================
// Procedural Creature Mesh (DX12 buffers)
// ========================================================================
struct CreatureVertexDX12 {
    float position[3];
    float padding1;
    float normal[3];
    float padding2;
    float texCoord[2];
};

static_assert(sizeof(CreatureVertexDX12) == 40, "CreatureVertexDX12 must be 40 bytes");

struct CreatureMeshDX12 {
    UniquePtr<IBuffer> vertexBuffer;
    UniquePtr<IBuffer> indexBuffer;
    u32 vertexCount = 0;
    u32 indexCount = 0;
    u32 vertexStride = 0;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);

    bool isValid() const {
        return vertexBuffer != nullptr && indexBuffer != nullptr && indexCount > 0;
    }
};

// ============================================================================
// Application State
// ============================================================================
struct AppState {
    // ForgeEngine objects
    UniquePtr<IWindow> window;
    UniquePtr<IDevice> device;
    UniquePtr<ISwapchain> swapchain;
    UniquePtr<ICommandList> commandList;
    UniquePtr<ICommandList> computeCommandList;
    UniquePtr<ITexture> depthBuffer;
    UniquePtr<IFence> frameFence;
    u64 fenceValue = 0;
    UniquePtr<IFence> computeFence;
    u64 computeFenceValue = 0;

    // GPU Steering
    std::unique_ptr<GPUSteeringCompute> gpuSteering;
    DX12DeviceAdapter dx12DeviceAdapter;
    bool gpuSteeringEnabled = false;
    bool gpuSteeringAvailable = false;
    float lastGPUComputeTime = 0.0f;
    u32 gpuSteeringDispatchCount = 0;

    // Simulation
    SimulationWorld world;
    DayNightCycle dayNight;

    // PHASE 10 - Agent 1: Unified creature management system
    std::unique_ptr<Forge::CreatureManager> creatureManager;
    BehaviorCoordinator behaviorCoordinator;
    SeasonManager seasonManager;
    ClimateSystem climateSystem;
    WeatherSystem weatherSystem;
    Forge::SimulationOrchestrator simulationOrchestrator;
    ui::GodModeUI godModeUI;
    bool useUnifiedSimulation = false;
    const Creature* followCreature = nullptr;
    std::mt19937 unifiedRng;

    // ImGui
    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    bool imguiInitialized = false;

    // Timing & Performance Profiling
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    FrameTimings timings;
    bool showPerformanceOverlay = true;

    // Camera (simple orbit camera)
    glm::vec3 cameraPosition = glm::vec3(0.0f, 100.0f, 200.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float cameraYaw = 0.0f;
    float cameraPitch = 30.0f;
    float cameraDistance = 200.0f;
    bool mouseCaptured = false;
    Camera camera;  // Phase 10: Camera object for selection system

    // Camera settings (configurable)
    bool invertMouseX = false;  // Horizontal mouse inversion
    bool invertMouseY = false;  // Vertical mouse inversion
    float mouseSensitivity = 0.15f;  // Mouse look sensitivity
    float cameraMoveSpeed = 120.0f;  // WASD movement speed (units/sec)
    float zoomSpeed = 15.0f;  // Zoom speed
    float minZoom = 10.0f;  // Minimum camera distance
    float maxZoom = 500.0f;  // Maximum camera distance

    // Creature following camera
    enum class CameraFollowMode { NONE, FOLLOW, ORBIT_CREATURE };
    CameraFollowMode cameraFollowMode = CameraFollowMode::NONE;
    int followCreatureId = -1;  // ID of creature being followed
    float followDistance = 30.0f;  // Distance from creature in follow mode
    float followHeight = 15.0f;  // Height above creature
    float followSmoothing = 5.0f;  // Smooth follow interpolation
    glm::vec3 followVelocity = glm::vec3(0.0f);  // For smooth damp
    float followOrbitAngle = 0.0f;  // Orbit angle around creature
    double lastClickTime = 0.0;  // For double-click detection

    // Camera presets
    enum class CameraPreset { FREE, OVERVIEW, GROUND, CINEMATIC };
    CameraPreset currentPreset = CameraPreset::FREE;

    // Cinematic camera
    float cinematicTime = 0.0f;
    bool cinematicPlaying = false;

    // Debug panel state
    bool showDebugPanel = true;
    bool showSteeringDebug = false;
    int selectedCreatureIndex = -1;

    // Creature info panel
    bool showCreatureInfoPanel = false;

    // Replay System (S-05, S-06 integration)
    Forge::ReplayRecorder replayRecorder;
    Forge::ReplayPlayer replayPlayer;
    bool isRecording = true;
    bool isPlayingReplay = false;
    std::string statusMessage;
    float statusMessageTimer = 0.0f;

    // Replay creature state for rendering during playback
    std::vector<SimCreature> replayCreatures;
    Forge::ReplayFrame currentReplayFrame;  // Current frame being displayed

    // Save/Load System (Phase 4 integration)
    Forge::SaveManager saveManager;
    bool showSaveLoadPanel = false;

    // Notification System (Phase 5 - Polish)
    NotificationManager notifications;

    // Gameplay Systems (Agent 26 - Fun Factor)
    Forge::GameplayManager gameplay;
    ui::GameplayUI gameplayUI;

    // Camera Transitions (Phase 5 - Smooth camera)
    MainCameraTransition cameraTransition;

    // Help Overlay (Phase 5 - UX)
    bool showHelpOverlay = false;
    bool showNametags = true;
    float nametagMaxDistance = 250.0f;

    // Loading state (Phase 5 - Loading screen)
    bool isLoading = false;
    float loadingProgress = 0.0f;
    std::string loadingStatus;
    bool worldGenInProgress = false;
    std::future<void> worldGenFuture;
    WorldGenConfig pendingProceduralConfig;
    ui::EvolutionStartPreset pendingEvolutionPreset;
    bool pendingGodMode = false;
    float loadingPulseTime = 0.0f;
    float worldGenElapsed = 0.0f;
    float worldGenProgress = 0.0f;
    std::string worldGenStage;
    std::mutex worldGenMutex;
    std::mutex worldGenLogMutex;
    bool worldDiagnostics = false;
    int worldDiagnosticsFrames = 0;

    // Phase 10 Observer UI (Agent 8 - Creature Inspection)
    ui::SelectionSystem selectionSystem;
    ui::CreatureInspectionPanel inspectionPanel;
    ui::MainMenu mainMenu;
    std::unique_ptr<ProceduralWorld> proceduralWorld;
    bool hasGeneratedWorld = false;
    bool godModeEnabled = false;

    // ========================================================================
    // Creature 3D Rendering Pipeline
    // ========================================================================
    UniquePtr<IShader> creatureVertexShader;
    UniquePtr<IShader> creaturePixelShader;
    UniquePtr<IPipeline> creaturePipeline;
    UniquePtr<IBuffer> creatureVertexBuffer;
    UniquePtr<IBuffer> creatureIndexBuffer;
    UniquePtr<IBuffer> creatureConstantBuffer;   // Combined scene + per-object data
    u32 creatureSphereIndexCount = 0;
    u32 creatureSphereVertexCount = 0;
    bool creatureRenderingInitialized = false;
    std::unique_ptr<CreatureMeshCache> creatureMeshCache;
    std::unordered_map<MeshKey, CreatureMeshDX12> creatureMeshDx12;

    // Combined constant buffer - scene + per-object data in single CB
    // Padded to 256 bytes for DX12 alignment
    struct alignas(256) CreatureConstants {
        float viewProj[16];      // 64 bytes - view-projection matrix
        float viewPos[4];        // 16 bytes - camera position (w unused)
        float lightDir[4];       // 16 bytes - light direction (w unused)
        float lightColor[4];     // 16 bytes - light color (w = intensity)
        float time;              // 4 bytes
        float padding[3];        // 12 bytes padding
        float objectPos[4];      // 16 bytes - per-object position (xyz) + unused (w)
        float objectScale[4];    // 16 bytes - per-object scale (xyz) + unused (w)
        float objectColor[4];    // 16 bytes - per-object color (rgba)
        float objectDir[4];      // 16 bytes - movement direction for body orientation (xyz) + unused (w)
        float padding2[16];      // 64 bytes to reach 256 total
    };

    // Maximum creatures we can render with offset-based CB approach
    static constexpr u32 MAX_CB_CREATURES = 2048;

    // Per-instance data (for collecting creature data)
    struct CreatureInstance {
        float position[3];       // World position
        float scale[3];          // Non-uniform scale (x, y, z) for body elongation
        float color[4];          // RGBA color
        float velocity[3];       // Velocity direction (for orientation)
    };

    static constexpr u32 MAX_CREATURE_INSTANCES = 16384;

    // ========================================================================
    // Water Rendering System (Phase 5 - 3D World)
    // ========================================================================
    WaterRenderer waterRenderer;
    bool waterRenderingEnabled = false;  // Disabled for testing
    float waterLevel = 0.0f;  // Water height in world units

    // ========================================================================
    // Grass Rendering System (Phase 5 - 3D World)
    // ========================================================================
    std::unique_ptr<GrassRendererDX12> grassRenderer;
    std::unique_ptr<GrassSystem> grassSystem;
    UniquePtr<IShader> grassVertexShader;
    UniquePtr<IShader> grassPixelShader;
    UniquePtr<IPipeline> grassPipeline;
    bool grassRenderingEnabled = false;  // Disabled for testing

    // ========================================================================
    // Tree Rendering System (Phase 5 - 3D World)
    // ========================================================================
    std::unique_ptr<TreeRendererDX12> treeRenderer;
    std::unique_ptr<VegetationManager> vegetationManager;
    std::unique_ptr<Terrain> terrain;  // Needed by VegetationManager
    UniquePtr<IShader> treeVertexShader;
    UniquePtr<IShader> treePixelShader;
    UniquePtr<IPipeline> treePipeline;
    bool treeRenderingEnabled = false;  // Disabled for testing

    // ========================================================================
    // Terrain Rendering System (Phase 5 - 3D World)
    // ========================================================================
    std::unique_ptr<TerrainRendererDX12> terrainRenderer;
    UniquePtr<IShader> terrainVertexShader;
    UniquePtr<IShader> terrainPixelShader;
    UniquePtr<IPipeline> terrainPipeline;
    bool terrainRenderingEnabled = false;  // Disabled for testing
};

static AppState g_app;

// ============================================================================
// Camera Transition Helper
// ============================================================================
void StartCameraTransition(const glm::vec3& pos, const glm::vec3& target, float duration) {
    g_app.cameraTransition.startPosition = g_app.cameraPosition;
    g_app.cameraTransition.startTarget = g_app.cameraTarget;
    g_app.cameraTransition.startYaw = g_app.cameraYaw;
    g_app.cameraTransition.startPitch = g_app.cameraPitch;
    g_app.cameraTransition.startDistance = g_app.cameraDistance;

    g_app.cameraTransition.startWithPosition(pos, target, duration);
}

static void ApplySettingsConfig(const ui::SettingsConfig& settings) {
    g_app.world.timeScale = settings.defaultSimSpeed;
    g_app.world.paused = settings.pauseOnStart;
    g_app.showNametags = settings.showNametags;
    g_app.nametagMaxDistance = settings.nametagDistance;
    g_app.invertMouseY = settings.invertY;
    g_app.mouseSensitivity = settings.cameraSensitivity * 0.15f;
    g_app.zoomSpeed = std::max(1.0f, settings.cameraSpeed * 0.15f);

    if (g_app.window) {
        g_app.window->SetVSync(settings.enableVSync);
    }

    if (settings.autoSave) {
        g_app.saveManager.enableAutoSave(static_cast<float>(settings.autoSaveInterval));
    } else {
        g_app.saveManager.disableAutoSave();
    }
}

static std::string GetLogTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return std::string(buffer);
}

static void AppendWorldGenMainLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_app.worldGenLogMutex);
    std::filesystem::create_directories("logs");
    std::ofstream logFile("logs/worldgen_main.log", std::ios::app);
    if (!logFile) {
        return;
    }
    logFile << "[" << GetLogTimestamp() << "] " << message << "\n";
}

static void AppendRuntimeDiagLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_app.worldGenLogMutex);
    std::filesystem::create_directories("logs");
    std::ofstream logFile("logs/runtime_diag.log", std::ios::app);
    if (!logFile) {
        return;
    }
    logFile << "[" << GetLogTimestamp() << "] " << message << "\n";
}

static void LogWorldDiag(const std::string& message) {
    if (!g_app.worldDiagnostics || g_app.worldDiagnosticsFrames <= 0) {
        return;
    }
    AppendRuntimeDiagLog(message);
}

static void SetLoadingStatus(const std::string& status, float progress) {
    g_app.loadingStatus = status;
    g_app.loadingProgress = std::max(g_app.loadingProgress, progress);
}

static void StartWorldGeneration(const ui::WorldGenConfig& menuConfig,
                                 const ui::EvolutionStartPreset& evolutionPreset,
                                 bool godModeEnabled) {
    if (g_app.worldGenInProgress) {
        return;
    }

    if (!g_app.proceduralWorld) {
        g_app.proceduralWorld = std::make_unique<ProceduralWorld>();
    }

    g_app.pendingProceduralConfig = ui::translateToProceduralWorldConfig(menuConfig);
    g_app.pendingEvolutionPreset = evolutionPreset;
    g_app.pendingGodMode = godModeEnabled;
    g_app.loadingPulseTime = 0.0f;
    g_app.worldGenElapsed = 0.0f;
    g_app.isLoading = true;
    g_app.loadingProgress = 0.0f;
    SetLoadingStatus("Preparing world generation...", 0.05f);
    {
        std::lock_guard<std::mutex> lock(g_app.worldGenMutex);
        g_app.worldGenProgress = 0.05f;
        g_app.worldGenStage = "Preparing world generation...";
    }
    g_app.worldGenInProgress = true;
    g_app.hasGeneratedWorld = false;
    ProceduralWorld* proceduralWorld = g_app.proceduralWorld.get();
    WorldGenConfig configCopy = g_app.pendingProceduralConfig;
    AppendWorldGenMainLog("World generation started.");
    AppendWorldGenMainLog(
        "Config: seed=" + std::to_string(configCopy.seed) +
        " resolution=" + std::to_string(configCopy.heightmapResolution) +
        " islands=" + std::to_string(configCopy.desiredRegionCount) +
        " ocean=" + std::to_string(configCopy.oceanCoverage) +
        " terrainScale=" + std::to_string(configCopy.terrainScale)
    );

    AppState* app = &g_app;
    proceduralWorld->setProgressCallback([app](float progress, const char* stage) {
        std::lock_guard<std::mutex> lock(app->worldGenMutex);
        app->worldGenProgress = std::max(app->worldGenProgress, progress);
        app->worldGenStage = stage ? stage : "";
    });

    g_app.worldGenFuture = std::async(std::launch::async, [proceduralWorld, configCopy]() {
        proceduralWorld->generate(configCopy);
    });
}

static void ApplyGeneratedWorldData(const WorldGenConfig& proceduralConfig,
                                    const ui::EvolutionStartPreset& evolutionPreset,
                                    bool godModeEnabled) {
    if (g_app.device) {
        g_app.device->WaitIdle();
        AppendWorldGenMainLog("GPU idle before world setup.");
    }

    const GeneratedWorld* world = g_app.proceduralWorld
        ? g_app.proceduralWorld->getCurrentWorld()
        : nullptr;
    if (!world) {
        g_app.isLoading = false;
        g_app.worldGenInProgress = false;
        g_app.mainMenu.setActive(true);
        g_app.statusMessage = "World generation failed";
        g_app.statusMessageTimer = 4.0f;
        AppendWorldGenMainLog("World generation failed: null world.");
        return;
    }

    if (!world->biomeSystem || !world->planetTheme) {
        g_app.isLoading = false;
        g_app.worldGenInProgress = false;
        g_app.mainMenu.setActive(true);
        g_app.statusMessage = "World generation incomplete";
        g_app.statusMessageTimer = 4.0f;
        AppendWorldGenMainLog("World generation failed: missing biome system or planet theme.");
        return;
    }
    if (world->islandData.width <= 0 || world->islandData.height <= 0 ||
        world->islandData.heightmap.empty()) {
        g_app.isLoading = false;
        g_app.worldGenInProgress = false;
        g_app.mainMenu.setActive(true);
        g_app.statusMessage = "World generation invalid terrain data";
        g_app.statusMessageTimer = 4.0f;
        AppendWorldGenMainLog("World generation failed: invalid terrain data.");
        return;
    }

    SetLoadingStatus("Finalizing world...", 0.9f);
    AppendWorldGenMainLog("Finalizing generated world.");

    const float worldSize = proceduralConfig.terrainScale;
    const float heightScale = 30.0f;
    const float waterLevel = world->islandData.params.waterLevel;
    const float beachLevel = std::min(0.95f, std::max(waterLevel + 0.02f, waterLevel + 0.07f));

    SetLoadingStatus("Initializing terrain sampler...", 0.91f);
    AppendWorldGenMainLog("Initializing terrain sampler.");
    TerrainSampler::SetWorldParams(worldSize, heightScale, waterLevel, beachLevel);
    TerrainSampler::SetHeightmap(&world->islandData.heightmap,
                                 world->islandData.width,
                                 world->islandData.height);

    SetLoadingStatus("Building terrain...", 0.92f);
    AppendWorldGenMainLog("Building terrain instance.");
    const float terrainScale = worldSize / static_cast<float>(std::max(1, world->islandData.width));
    g_app.terrain = std::make_unique<Terrain>(world->islandData.width, world->islandData.height, terrainScale);
    g_app.terrain->generate(world->planetSeed.terrainSeed);
    AppendWorldGenMainLog("Terrain instance ready.");

    g_app.world.terrainSeed = world->planetSeed.terrainSeed;
    g_app.world.SetWorldBounds(worldSize * 0.5f);
    g_app.creatureManager = std::make_unique<Forge::CreatureManager>(worldSize, worldSize);
    g_app.creatureManager->init(g_app.terrain.get(), nullptr, world->planetSeed.terrainSeed);
    g_app.behaviorCoordinator.init(g_app.creatureManager.get(),
                                   g_app.creatureManager->getGlobalGrid(),
                                   nullptr,
                                   &g_app.seasonManager,
                                   world->biomeSystem.get(),
                                   g_app.terrain.get());
    g_app.behaviorCoordinator.reset();
    AppendWorldGenMainLog("Creature manager initialized.");

    SetLoadingStatus("Generating vegetation...", 0.94f);
    AppendWorldGenMainLog("Generating vegetation.");
    g_app.vegetationManager = std::make_unique<VegetationManager>(g_app.terrain.get());
    g_app.vegetationManager->generate(world->planetSeed.vegetationSeed);
    g_app.vegetationManager->initializeAquaticPlants(nullptr, world->planetSeed.vegetationSeed);
    AppendWorldGenMainLog("Vegetation generated.");

    SetLoadingStatus("Generating grass...", 0.95f);
    AppendWorldGenMainLog("Generating grass.");
    g_app.grassSystem = std::make_unique<GrassSystem>();
    g_app.grassSystem->initialize(nullptr, g_app.terrain.get());
    g_app.grassSystem->generate(world->planetSeed.vegetationSeed);
    AppendWorldGenMainLog("Grass generated.");

    if (g_app.grassPipeline) {
        AppendWorldGenMainLog("Initializing grass renderer.");
        g_app.grassRenderer = std::make_unique<GrassRendererDX12>();
        g_app.grassRenderingEnabled = g_app.grassRenderer->init(g_app.device.get(), g_app.grassSystem.get());
        AppendWorldGenMainLog(std::string("Grass renderer init: ") +
                              (g_app.grassRenderingEnabled ? "ok" : "failed"));
    }

    if (g_app.treePipeline) {
        SetLoadingStatus("Generating trees...", 0.96f);
        AppendWorldGenMainLog("Initializing tree renderer.");
        g_app.treeRenderer = std::make_unique<TreeRendererDX12>();
        if (g_app.treeRenderer->init(g_app.device.get(), g_app.vegetationManager.get())) {
            AppendWorldGenMainLog("Generating tree meshes...");
            g_app.treeRenderer->generateTreeMeshes();
            g_app.treeRenderingEnabled = true;
            AppendWorldGenMainLog("Tree meshes generated.");
        } else {
            g_app.treeRenderingEnabled = false;
            AppendWorldGenMainLog("Tree renderer init failed.");
        }
    }

    if (g_app.terrainPipeline) {
        SetLoadingStatus("Preparing terrain renderer...", 0.97f);
        AppendWorldGenMainLog("Initializing terrain renderer.");
        g_app.terrainRenderer = std::make_unique<TerrainRendererDX12>();
        g_app.terrainRenderingEnabled = g_app.terrainRenderer->init(g_app.device.get());
        AppendWorldGenMainLog(std::string("Terrain renderer init: ") +
                              (g_app.terrainRenderingEnabled ? "ok" : "failed"));
    }

    SetLoadingStatus("Preparing water...", 0.975f);
    g_app.waterLevel = TerrainSampler::GetWaterHeight();
    if (g_app.waterRenderer.IsInitialized()) {
        g_app.waterRenderer.GenerateMesh(64, worldSize, g_app.waterLevel);

        if (world->planetTheme) {
            const auto& terrainPalette = world->planetTheme->getTerrain();
            g_app.waterRenderer.SetWaterColor(
                glm::vec4(terrainPalette.deepWaterColor, 1.0f),
                glm::vec4(terrainPalette.shallowWaterColor, 1.0f)
            );
            auto atmosphere = world->planetTheme->getCurrentAtmosphere();
            g_app.waterRenderer.SetSkyColors(atmosphere.skyZenithColor, atmosphere.skyHorizonColor);
        }
    }

    g_app.inspectionPanel.setBiomeSystem(world->biomeSystem.get());

    ApplySettingsConfig(g_app.mainMenu.getSettings());

    SetLoadingStatus("Initializing climate systems...", 0.98f);
    g_app.useUnifiedSimulation = true;
    g_app.unifiedRng.seed(world->planetSeed.terrainSeed);

    g_app.seasonManager = SeasonManager();
    g_app.climateSystem = ClimateSystem();
    g_app.weatherSystem = WeatherSystem();
    g_app.climateSystem.initialize(g_app.terrain.get(), &g_app.seasonManager);
    g_app.weatherSystem.initialize(&g_app.seasonManager, &g_app.climateSystem);

    if (g_app.vegetationManager) {
        g_app.vegetationManager->setClimateSystem(&g_app.climateSystem);
    }
    if (g_app.grassSystem) {
        g_app.grassSystem->setClimateSystem(&g_app.climateSystem);
    }

    SetLoadingStatus("Spawning life...", 0.99f);
    g_app.world.simulationTime = 0.0f;
    g_app.world.maxGeneration = 1;
    g_app.world.totalBirths = 0;
    g_app.world.totalDeaths = 0;
    g_app.world.creaturePool.clear();
    g_app.world.activeCreatures.clear();
    g_app.world.creatures.clear();
    g_app.world.UpdateStats();

    ResetUnifiedFoodSources(static_cast<u32>(std::max(0, evolutionPreset.plantCount)));
    SpawnInitialCreatures(evolutionPreset);

    g_app.simulationOrchestrator.bindTimeState(&g_app.world.paused,
                                               &g_app.world.timeScale,
                                               &g_app.world.simulationTime);
    g_app.simulationOrchestrator.setCreatureManager(g_app.creatureManager.get());
    g_app.simulationOrchestrator.setTerrain(g_app.terrain.get());
    g_app.simulationOrchestrator.setWeather(&g_app.weatherSystem);
    g_app.simulationOrchestrator.setStepFramesCallback([](int count) {
        StepUnifiedSimulation(count);
    });

    g_app.godModeEnabled = godModeEnabled;
    g_app.godModeUI.init(&g_app.simulationOrchestrator);
    g_app.godModeUI.setEnabled(g_app.godModeEnabled);
    g_app.followCreature = nullptr;
    g_app.followCreatureId = -1;
    g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
    g_app.hasGeneratedWorld = true;
    g_app.mainMenu.setCanContinue(true);

    SetLoadingStatus("World ready", 1.0f);
    g_app.isLoading = false;
    g_app.worldGenInProgress = false;
    g_app.mainMenu.setActive(false);
    AppendWorldGenMainLog("World generation completed.");
    g_app.worldDiagnostics = true;
    g_app.worldDiagnosticsFrames = 3;
    AppendRuntimeDiagLog("Diagnostics armed: next 3 frames.");
}

static float GetCreatureRenderSurfaceHeight(const Creature& creature) {
    float surfaceHeight = creature.getPosition().y;
    if (!isFlying(creature.getType()) && !isAquatic(creature.getType())) {
        surfaceHeight -= creature.getGenome().size;
    }
    return surfaceHeight;
}

static void ResetUnifiedFoodSources(u32 plantCount) {
    g_app.world.foods.clear();
    if (plantCount == 0) {
        return;
    }

    float spawnRadius = g_app.world.GetWorldBounds() * 0.95f;
    g_app.world.SpawnFood(plantCount, spawnRadius, 40.0f, 60.0f);
}

static void SpawnInitialCreatures(const ui::EvolutionStartPreset& evolutionPreset) {
    if (!g_app.creatureManager) {
        return;
    }

    float spawnRadius = g_app.world.GetWorldBounds() * 0.9f;
    std::uniform_real_distribution<float> posDist(-spawnRadius, spawnRadius);

    const std::array<CreatureType, 3> flyingTypes = {
        CreatureType::FLYING_BIRD,
        CreatureType::FLYING_INSECT,
        CreatureType::AERIAL_PREDATOR
    };
    const std::array<CreatureType, 3> herbivoreTypes = {
        CreatureType::GRAZER,
        CreatureType::BROWSER,
        CreatureType::FRUGIVORE
    };
    const std::array<CreatureType, 3> carnivoreTypes = {
        CreatureType::SMALL_PREDATOR,
        CreatureType::OMNIVORE,
        CreatureType::APEX_PREDATOR
    };
    const std::array<CreatureType, 3> aquaticTypes = {
        CreatureType::AQUATIC_HERBIVORE,
        CreatureType::AQUATIC_PREDATOR,
        CreatureType::AQUATIC_APEX
    };
    std::uniform_int_distribution<size_t> flyingTypeDist(0, flyingTypes.size() - 1);
    std::uniform_int_distribution<size_t> herbivoreTypeDist(0, herbivoreTypes.size() - 1);
    std::uniform_int_distribution<size_t> carnivoreTypeDist(0, carnivoreTypes.size() - 1);
    std::uniform_int_distribution<size_t> aquaticTypeDist(0, aquaticTypes.size() - 1);

    auto spawnBatch = [&](CreatureType type, int count) {
        for (int i = 0; i < count; ++i) {
            glm::vec3 pos(posDist(g_app.unifiedRng), 0.0f, posDist(g_app.unifiedRng));
            Forge::CreatureHandle handle = g_app.creatureManager->spawn(type, pos, nullptr);
            if (Creature* creature = g_app.creatureManager->get(handle)) {
                creature->setGeneration(1);
            }
        }
    };

    for (int i = 0; i < std::max(0, evolutionPreset.herbivoreCount); ++i) {
        CreatureType type = herbivoreTypes[herbivoreTypeDist(g_app.unifiedRng)];
        spawnBatch(type, 1);
    }

    for (int i = 0; i < std::max(0, evolutionPreset.carnivoreCount); ++i) {
        CreatureType type = carnivoreTypes[carnivoreTypeDist(g_app.unifiedRng)];
        spawnBatch(type, 1);
    }

    for (int i = 0; i < evolutionPreset.flyingCount; ++i) {
        CreatureType type = flyingTypes[flyingTypeDist(g_app.unifiedRng)];
        spawnBatch(type, 1);
    }

    for (int i = 0; i < evolutionPreset.aquaticCount; ++i) {
        CreatureType type = aquaticTypes[aquaticTypeDist(g_app.unifiedRng)];
        spawnBatch(type, 1);
    }
}

static void UpdateUnifiedSimulation(float dt) {
    if (!g_app.creatureManager || !g_app.terrain) {
        return;
    }

    if (g_app.world.paused) {
        return;
    }

    LogWorldDiag("Unified step: start");

    float scaledDt = dt * g_app.world.timeScale;
    g_app.world.simulationTime += scaledDt;

    g_app.seasonManager.update(scaledDt);
    g_app.climateSystem.update(scaledDt);
    g_app.weatherSystem.update(scaledDt);
    LogWorldDiag("Unified step: climate/weather updated");

    EnvironmentConditions env;
    const WeatherState weather = g_app.weatherSystem.getInterpolatedWeather();
    env.visibility = std::clamp(1.0f - weather.fogDensity, 0.1f, 1.0f);
    env.ambientLight = std::clamp(weather.sunIntensity, 0.1f, 1.0f);
    glm::vec3 windDir(weather.windDirection.x, 0.0f, weather.windDirection.y);
    if (glm::length(windDir) > 0.001f) {
        env.windDirection = glm::normalize(windDir);
    }
    env.windSpeed = weather.windStrength * 10.0f;
    env.temperature = g_app.climateSystem.getGlobalTemperature() + weather.temperatureModifier;

    std::vector<glm::vec3> foodPositions;
    foodPositions.reserve(g_app.world.foods.size());
    for (const auto& food : g_app.world.foods) {
        if (food->amount > 0.0f) {
            foodPositions.push_back(food->position);
        }
    }
    if (g_app.worldDiagnostics && g_app.worldDiagnosticsFrames > 0) {
        LogWorldDiag("Unified step: food positions=" + std::to_string(foodPositions.size()));
    }

    std::vector<Creature*> creatures;
    creatures.reserve(g_app.creatureManager->getAllCreatures().size());
    g_app.creatureManager->forEach([&](Creature& creature, size_t) {
        creatures.push_back(&creature);
    });
    if (g_app.worldDiagnostics && g_app.worldDiagnosticsFrames > 0) {
        LogWorldDiag("Unified step: creatures=" + std::to_string(creatures.size()));
    }

    g_app.creatureManager->rebuildSpatialGrids();
    g_app.behaviorCoordinator.update(scaledDt);
    LogWorldDiag("Unified step: spatial grids rebuilt + behavior updated");

    struct ReproCandidate {
        CreatureType type;
        glm::vec3 position;
        Genome genome;
        int generation = 0;
    };
    std::vector<ReproCandidate> reproQueue;
    reproQueue.reserve(64);

    std::uniform_real_distribution<float> reproChance(0.0f, 1.0f);
    const float reproRate = 0.015f;

    int debugLogged = 0;
    for (Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) {
            continue;
        }

        if (debugLogged < 3) {
            LogWorldDiag("Unified creature update begin id=" + std::to_string(creature->getID()));
        }
        creature->update(
            scaledDt,
            *g_app.terrain,
            foodPositions,
            creatures,
            g_app.creatureManager->getGlobalGrid(),
            &env,
            nullptr,
            &g_app.behaviorCoordinator
        );
        if (debugLogged < 3) {
            LogWorldDiag("Unified creature update end id=" + std::to_string(creature->getID()));
        }

        ClimateData climate = g_app.climateSystem.getClimateAt(creature->getPosition());
        if (debugLogged < 3) {
            LogWorldDiag("Unified climate response begin id=" + std::to_string(creature->getID()));
        }
        creature->updateClimateResponse(climate, &g_app.climateSystem, scaledDt);
        if (debugLogged < 3) {
            LogWorldDiag("Unified climate response end id=" + std::to_string(creature->getID()));
        }

        if (creature->canReproduce() && reproChance(g_app.unifiedRng) < reproRate * scaledDt) {
            float energyCost = 0.0f;
            creature->reproduce(energyCost);
            reproQueue.push_back({
                creature->getType(),
                creature->getPosition(),
                creature->getGenome(),
                creature->getGeneration()
            });
        }

        if (debugLogged < 3) {
            debugLogged++;
        }
    }
    LogWorldDiag("Unified step: creature updates done");

    constexpr float FOOD_EAT_RANGE_SQ = 4.0f;
    for (Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) {
            continue;
        }

        if (!isHerbivore(creature->getType()) && creature->getType() != CreatureType::FLYING) {
            continue;
        }

        for (auto& food : g_app.world.foods) {
            if (food->amount <= 0.0f) {
                continue;
            }

            float dx = food->position.x - creature->getPosition().x;
            float dz = food->position.z - creature->getPosition().z;
            float distSq = dx * dx + dz * dz;

            if (distSq < FOOD_EAT_RANGE_SQ) {
                float eatAmount = std::min(food->amount, 10.0f * scaledDt);
                creature->consumeFood(eatAmount);
                food->amount -= eatAmount;
                break;
            }
        }
    }
    LogWorldDiag("Unified step: food consumption done");

    g_app.world.foods.erase(
        std::remove_if(g_app.world.foods.begin(), g_app.world.foods.end(),
                       [](const std::unique_ptr<Food>& food) { return food->amount <= 0.0f; }),
        g_app.world.foods.end()
    );
    LogWorldDiag("Unified step: food cleanup done");

    if (!reproQueue.empty()) {
        for (const auto& entry : reproQueue) {
            Forge::CreatureHandle handle = g_app.creatureManager->spawn(
                entry.type,
                entry.position,
                &entry.genome
            );
            if (Creature* child = g_app.creatureManager->get(handle)) {
                child->setGeneration(entry.generation + 1);
            }
        }
    }
    LogWorldDiag("Unified step: reproduction done");

    std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
    float spawnBound = std::max(1.0f, g_app.world.GetWorldBounds());
    std::uniform_real_distribution<float> posDist(-spawnBound, spawnBound);

    if (spawnChance(g_app.world.respawnRng) < 0.1f * scaledDt &&
        g_app.world.foods.size() < MAX_FOOD_SOURCES) {
        for (int attempt = 0; attempt < 5; ++attempt) {
            float x = posDist(g_app.world.respawnRng);
            float z = posDist(g_app.world.respawnRng);
            if (TerrainSampler::IsWater(x, z)) {
                continue;
            }
            float y = TerrainSampler::SampleHeight(x, z);
            g_app.world.foods.push_back(std::make_unique<Food>(glm::vec3(x, y, z), 50.0f));
            break;
        }
    }
    LogWorldDiag("Unified step: food respawn done");

    int maxCreatures = std::max(10, g_app.mainMenu.getSettings().maxCreatures);
    g_app.creatureManager->cullToLimit(static_cast<size_t>(maxCreatures));
    LogWorldDiag("Unified step: cull done");

    g_app.creatureManager->updateAmphibiousTransitions(scaledDt, TerrainSampler::GetWaterHeight());
    g_app.creatureManager->update(scaledDt);
    LogWorldDiag("Unified step: amphibious + manager update done");
}

static void StepUnifiedSimulation(int count) {
    if (count <= 0) {
        return;
    }

    const float stepDt = 1.0f / 60.0f;
    for (int i = 0; i < count; ++i) {
        UpdateUnifiedSimulation(stepDt);
    }
}

// Helper to get creature position by index from creature pool
glm::vec3 GetCreaturePosition(int index) {
    if (index < 0) return glm::vec3(0.0f);

    auto activeCreatures = g_app.world.creaturePool.getActiveCreatures();
    if (index < static_cast<int>(activeCreatures.size())) {
        const SimCreature* c = activeCreatures[index];
        return c->position;
    }
    return glm::vec3(0.0f);
}

// Helper to get creature velocity by index
glm::vec3 GetCreatureVelocity(int index) {
    if (index < 0) return glm::vec3(0.0f, 0.0f, 1.0f);

    auto activeCreatures = g_app.world.creaturePool.getActiveCreatures();
    if (index < static_cast<int>(activeCreatures.size())) {
        const SimCreature* c = activeCreatures[index];
        glm::vec3 vel = c->velocity;
        if (glm::length(vel) < 0.01f) {
            vel = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        return glm::normalize(vel);
    }
    return glm::vec3(0.0f, 0.0f, 1.0f);
}

// SmoothDamp for glm::vec3 (Unity-style)
glm::vec3 SmoothDamp(const glm::vec3& current, const glm::vec3& target,
                      glm::vec3& velocity, float smoothTime, float deltaTime) {
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    glm::vec3 change = current - target;
    glm::vec3 temp = (velocity + omega * change) * deltaTime;
    velocity = (velocity - omega * temp) * exp;

    glm::vec3 result = target + (change + temp) * exp;

    // Prevent overshooting
    if (glm::dot(target - current, result - target) > 0.0f) {
        result = target;
        velocity = glm::vec3(0.0f);
    }

    return result;
}

// ============================================================================
// Creature Rendering - HLSL Shader Source
// ============================================================================
// Enhanced creature shader with non-uniform scaling and body orientation
static const char* g_creatureVertexShaderSource = R"(
cbuffer Constants : register(b0) {
    float4x4 viewProj;
    float4 viewPos;
    float4 lightDir;
    float4 lightColor;
    float time;
    float3 padding;
    // Per-object data packed at end of CB0 (avoids second CB issues)
    float4 objectPos;    // xyz = position, w = unused
    float4 objectScale;  // xyz = non-uniform scale, w = phase offset
    float4 objectColor;  // rgb + type tag in a
    float4 objectDir;    // xyz = movement direction, w = speed
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD0;
    float3 color : COLOR;
};

PSInput main(VSInput input) {
    PSInput output;

    // Build rotation matrix from movement direction
    float3 forward = objectDir.xyz;
    float dirLength = length(forward);

    // If creature is moving, orient body in movement direction
    // Otherwise use default forward direction (0, 0, 1)
    float3x3 rotation;
    if (dirLength > 0.01) {
        forward = normalize(forward);

        // Create orthonormal basis with forward as Z axis
        float3 up = float3(0, 1, 0);

        // Handle case where forward is nearly parallel to up
        if (abs(dot(forward, up)) > 0.99) {
            up = float3(1, 0, 0);
        }

        float3 right = normalize(cross(up, forward));
        up = normalize(cross(forward, right));

        // Rotation matrix: columns are right, up, forward
        rotation = float3x3(right, up, forward);
    } else {
        // Identity rotation when not moving
        rotation = float3x3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        );
    }

    // Apply non-uniform scaling first (in local space)
    // Scale: x = width, y = height, z = length
    float3 scaledPos = input.position * objectScale.xyz;

    float speed = objectDir.w;
    float typeTag = objectColor.a;
    float flyMask = step(0.5f, typeTag) - step(1.5f, typeTag);
    float swimMask = step(1.5f, typeTag);
    float groundMask = 1.0f - flyMask - swimMask;

    float phase = time * (2.5f + speed * 0.15f) + objectPos.x * 0.05f + objectPos.z * 0.05f + objectScale.w * 6.28318f;
    float bob = sin(phase) * 0.02f * saturate(speed * 0.1f);
    float swimWave = sin(phase + input.position.x * 3.0f) * 0.05f * swimMask;
    float flap = sin(phase * 2.0f) * 0.12f * flyMask * abs(input.position.z);

    scaledPos.y += bob * groundMask;
    scaledPos.y += swimWave;
    scaledPos.y += flap;

    // Rotate to align with movement direction, then translate
    float3 rotatedPos = mul(rotation, scaledPos);
    float3 worldPos = rotatedPos + objectPos.xyz;

    output.position = mul(viewProj, float4(worldPos, 1.0));
    output.worldPos = worldPos;

    // Transform normal (inverse transpose of rotation, but since it's orthonormal, just use rotation)
    float3 scaledNormal = input.normal / objectScale.xyz;  // Account for non-uniform scale
    scaledNormal = normalize(scaledNormal);
    output.normal = normalize(mul(rotation, scaledNormal));

    // Enhanced lighting with specular highlights
    float3 L = normalize(-lightDir.xyz);
    float3 V = normalize(viewPos.xyz - worldPos);
    float3 H = normalize(L + V);

    // Diffuse lighting with wrap-around for softer look
    float ndotl = dot(output.normal, L);
    float diffuse = max(ndotl * 0.5 + 0.5, 0.0);  // Half-lambert for softer shading

    // Specular highlight
    float ndoth = max(dot(output.normal, H), 0.0);
    float specular = pow(ndoth, 32.0) * 0.3;

    // Rim lighting for better silhouette visibility
    float rim = 1.0 - max(dot(output.normal, V), 0.0);
    rim = pow(rim, 3.0) * 0.2;

    // Ambient + diffuse + specular + rim
    float lighting = 0.25 + 0.6 * diffuse + specular + rim;

    output.color = objectColor.rgb * lighting;
    return output;
}
)";

static const char* g_creaturePixelShaderSource = R"(
struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD0;
    float3 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET {
    // Apply slight color saturation boost for vibrancy
    float3 color = input.color;

    // Add subtle fresnel-like edge brightening for better shape definition
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    float saturationBoost = 1.1;
    color = lerp(float3(luminance, luminance, luminance), color, saturationBoost);

    // Ensure colors stay in valid range
    color = saturate(color);

    return float4(color, 1.0);
}
)";

// ============================================================================
// ============================================================================
// SIMPLE GROUND PLANE - Test basic rendering
// ============================================================================
static const char* g_groundVertexShader = R"(
cbuffer Constants : register(b0) {
    float4x4 viewProj;
    float4 viewPos;
    float4 lightDir;
    float4 lightColor;
    float time;
    float3 padding;
};

struct VSInput {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

PSInput main(VSInput input) {
    PSInput output;
    output.position = mul(viewProj, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}
)";

static const char* g_groundPixelShader = R"(
struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
)";

struct GroundVertex {
    float position[3];
    float color[3];
};

// Ground plane data (will be initialized)
static UniquePtr<IShader> g_groundVS;
static UniquePtr<IShader> g_groundPS;
static UniquePtr<IPipeline> g_groundPipeline;
static UniquePtr<IBuffer> g_groundVB;
static UniquePtr<IBuffer> g_groundIB;
static UniquePtr<IBuffer> g_groundCB;  // Ground plane constant buffer
static bool g_groundInitialized = false;

bool InitializeGroundPlane() {
    std::cout << "Initializing ground plane..." << std::endl;

    // Create shaders
    ShaderDesc vsDesc;
    vsDesc.type = ShaderType::Vertex;
    vsDesc.source = g_groundVertexShader;
    vsDesc.entryPoint = "main";
    vsDesc.debugName = "GroundVS";
    g_groundVS = g_app.device->CreateShader(vsDesc);
    if (!g_groundVS) {
        std::cerr << "  Failed to create ground vertex shader!" << std::endl;
        return false;
    }

    ShaderDesc psDesc;
    psDesc.type = ShaderType::Pixel;
    psDesc.source = g_groundPixelShader;
    psDesc.entryPoint = "main";
    psDesc.debugName = "GroundPS";
    g_groundPS = g_app.device->CreateShader(psDesc);
    if (!g_groundPS) {
        std::cerr << "  Failed to create ground pixel shader!" << std::endl;
        return false;
    }

    // Create pipeline
    PipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = g_groundVS.get();
    pipelineDesc.pixelShader = g_groundPS.get();

    VertexAttribute posAttr;
    posAttr.semanticName = "POSITION";
    posAttr.semanticIndex = 0;
    posAttr.format = Format::R32G32B32_FLOAT;
    posAttr.inputSlot = 0;
    posAttr.offset = 0;
    posAttr.inputRate = InputRate::PerVertex;
    pipelineDesc.vertexLayout.push_back(posAttr);

    VertexAttribute colorAttr;
    colorAttr.semanticName = "COLOR";
    colorAttr.semanticIndex = 0;
    colorAttr.format = Format::R32G32B32_FLOAT;
    colorAttr.inputSlot = 0;
    colorAttr.offset = 12;
    colorAttr.inputRate = InputRate::PerVertex;
    pipelineDesc.vertexLayout.push_back(colorAttr);

    pipelineDesc.primitiveTopology = PrimitiveTopology::TriangleList;
    pipelineDesc.cullMode = CullMode::None;  // Render both sides
    pipelineDesc.depthTestEnabled = true;
    pipelineDesc.depthWriteEnabled = true;
    pipelineDesc.depthCompareOp = CompareOp::Less;
    pipelineDesc.renderTargetFormats.push_back(Format::R8G8B8A8_UNORM);
    pipelineDesc.depthStencilFormat = Format::D32_FLOAT;
    pipelineDesc.debugName = "GroundPipeline";

    g_groundPipeline = g_app.device->CreatePipeline(pipelineDesc);
    if (!g_groundPipeline) {
        std::cerr << "  Failed to create ground pipeline!" << std::endl;
        return false;
    }

    // Create ground plane vertices - large grid
    float size = 500.0f;
    float y = 0.0f;
    GroundVertex vertices[] = {
        // Position             // Color (green grass)
        {{-size, y, -size},    {0.2f, 0.5f, 0.2f}},
        {{ size, y, -size},    {0.3f, 0.6f, 0.3f}},
        {{ size, y,  size},    {0.2f, 0.5f, 0.2f}},
        {{-size, y,  size},    {0.3f, 0.6f, 0.3f}},
    };

    u32 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // Create vertex buffer
    BufferDesc vbDesc;
    vbDesc.size = sizeof(vertices);
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuAccess = true;
    vbDesc.debugName = "GroundVB";
    g_groundVB = g_app.device->CreateBuffer(vbDesc);
    if (!g_groundVB) {
        std::cerr << "  Failed to create ground vertex buffer!" << std::endl;
        return false;
    }
    void* vbData = g_groundVB->Map();
    if (vbData) {
        memcpy(vbData, vertices, sizeof(vertices));
        g_groundVB->Unmap();
    }

    // Create index buffer
    BufferDesc ibDesc;
    ibDesc.size = sizeof(indices);
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuAccess = true;
    ibDesc.debugName = "GroundIB";
    g_groundIB = g_app.device->CreateBuffer(ibDesc);
    if (!g_groundIB) {
        std::cerr << "  Failed to create ground index buffer!" << std::endl;
        return false;
    }
    void* ibData = g_groundIB->Map();
    if (ibData) {
        memcpy(ibData, indices, sizeof(indices));
        g_groundIB->Unmap();
    }

    // Create constant buffer for ground plane (256 bytes, DX12 aligned)
    BufferDesc cbDesc;
    cbDesc.size = 256;
    cbDesc.usage = BufferUsage::Uniform;
    cbDesc.cpuAccess = true;
    cbDesc.debugName = "GroundCB";
    g_groundCB = g_app.device->CreateBuffer(cbDesc);
    if (!g_groundCB) {
        std::cerr << "  Failed to create ground constant buffer!" << std::endl;
        return false;
    }

    g_groundInitialized = true;
    std::cout << "  Ground plane initialized!" << std::endl;
    return true;
}

void RenderGroundPlane(const glm::mat4& viewProj,
                       const glm::vec3& cameraPos,
                       const glm::vec3& lightDir,
                       const glm::vec3& lightColor) {
    if (!g_groundInitialized) return;

    static bool firstGroundDraw = true;
    if (firstGroundDraw) {
        std::cout << "[RenderGroundPlane] Drawing 2 triangles (6 indices)" << std::endl;
        firstGroundDraw = false;
    }

    // Update constant buffer with view/projection
    struct alignas(256) GroundConstants {
        float viewProj[16];
        float viewPos[4];
        float lightDir[4];
        float lightColor[4];
        float time;
        float padding[3];
    };

    GroundConstants constants;
    memcpy(constants.viewProj, glm::value_ptr(viewProj), sizeof(float) * 16);
    constants.viewPos[0] = cameraPos.x;
    constants.viewPos[1] = cameraPos.y;
    constants.viewPos[2] = cameraPos.z;
    constants.viewPos[3] = 1.0f;
    constants.lightDir[0] = lightDir.x;
    constants.lightDir[1] = lightDir.y;
    constants.lightDir[2] = lightDir.z;
    constants.lightDir[3] = 0.0f;
    constants.lightColor[0] = lightColor.x;
    constants.lightColor[1] = lightColor.y;
    constants.lightColor[2] = lightColor.z;
    constants.lightColor[3] = 1.0f;
    constants.time = g_app.world.simulationTime;
    constants.padding[0] = constants.padding[1] = constants.padding[2] = 0.0f;

    void* cbData = g_groundCB->Map();
    if (cbData) {
        memcpy(cbData, &constants, sizeof(constants));
        g_groundCB->Unmap();
    }

    g_app.commandList->SetPipeline(g_groundPipeline.get());
    g_app.commandList->BindConstantBuffer(0, g_groundCB.get());
    g_app.commandList->BindVertexBuffer(0, g_groundVB.get(), sizeof(GroundVertex), 0);
    g_app.commandList->BindIndexBuffer(g_groundIB.get(), IndexFormat::UInt32, 0);
    g_app.commandList->DrawIndexed(6, 0, 0);
}

// ============================================================================
// Creature Rendering - Sphere Mesh Generation
// ============================================================================
struct SphereVertex {
    float position[3];
    float normal[3];
};

void GenerateSphereMesh(std::vector<SphereVertex>& vertices, std::vector<u32>& indices,
                        u32 slices = 16, u32 stacks = 12, float radius = 1.0f) {
    vertices.clear();
    indices.clear();

    // Generate vertices
    for (u32 stack = 0; stack <= stacks; ++stack) {
        float phi = 3.14159265f * float(stack) / float(stacks);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (u32 slice = 0; slice <= slices; ++slice) {
            float theta = 2.0f * 3.14159265f * float(slice) / float(slices);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            SphereVertex v;
            v.normal[0] = sinPhi * cosTheta;
            v.normal[1] = cosPhi;
            v.normal[2] = sinPhi * sinTheta;
            v.position[0] = radius * v.normal[0];
            v.position[1] = radius * v.normal[1];
            v.position[2] = radius * v.normal[2];

            vertices.push_back(v);
        }
    }

    // Generate indices
    for (u32 stack = 0; stack < stacks; ++stack) {
        for (u32 slice = 0; slice < slices; ++slice) {
            u32 first = stack * (slices + 1) + slice;
            u32 second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

// ============================================================================
// Creature Rendering - Pipeline Initialization
// ============================================================================
bool InitializeCreatureRendering() {
    std::cout << "Initializing creature 3D rendering pipeline..." << std::endl;

    if (!g_app.device) {
        std::cerr << "  ERROR: Device not available!" << std::endl;
        return false;
    }

    // Create vertex shader
    ShaderDesc vsDesc;
    vsDesc.type = ShaderType::Vertex;
    vsDesc.source = g_creatureVertexShaderSource;
    vsDesc.entryPoint = "main";
    vsDesc.debugName = "CreatureVS";

    g_app.creatureVertexShader = g_app.device->CreateShader(vsDesc);
    if (!g_app.creatureVertexShader) {
        std::cerr << "  ERROR: Failed to create creature vertex shader!" << std::endl;
        return false;
    }
    std::cout << "  Vertex shader compiled" << std::endl;

    // Create pixel shader
    ShaderDesc psDesc;
    psDesc.type = ShaderType::Pixel;
    psDesc.source = g_creaturePixelShaderSource;
    psDesc.entryPoint = "main";
    psDesc.debugName = "CreaturePS";

    g_app.creaturePixelShader = g_app.device->CreateShader(psDesc);
    if (!g_app.creaturePixelShader) {
        std::cerr << "  ERROR: Failed to create creature pixel shader!" << std::endl;
        return false;
    }
    std::cout << "  Pixel shader compiled" << std::endl;

    // Create graphics pipeline
    PipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = g_app.creatureVertexShader.get();
    pipelineDesc.pixelShader = g_app.creaturePixelShader.get();

    // Vertex layout: per-vertex data (slot 0)
    VertexAttribute posAttr;
    posAttr.semanticName = "POSITION";
    posAttr.semanticIndex = 0;
    posAttr.format = Format::R32G32B32_FLOAT;
    posAttr.inputSlot = 0;
    posAttr.offset = 0;
    posAttr.inputRate = InputRate::PerVertex;
    pipelineDesc.vertexLayout.push_back(posAttr);

    VertexAttribute normalAttr;
    normalAttr.semanticName = "NORMAL";
    normalAttr.semanticIndex = 0;
    normalAttr.format = Format::R32G32B32_FLOAT;
    normalAttr.inputSlot = 0;
    normalAttr.offset = 16;
    normalAttr.inputRate = InputRate::PerVertex;
    pipelineDesc.vertexLayout.push_back(normalAttr);

    VertexAttribute texAttr;
    texAttr.semanticName = "TEXCOORD";
    texAttr.semanticIndex = 0;
    texAttr.format = Format::R32G32_FLOAT;
    texAttr.inputSlot = 0;
    texAttr.offset = 32;
    texAttr.inputRate = InputRate::PerVertex;
    pipelineDesc.vertexLayout.push_back(texAttr);

    // No per-instance data - we'll use constant buffer for per-object transforms

    pipelineDesc.primitiveTopology = PrimitiveTopology::TriangleList;
    pipelineDesc.cullMode = CullMode::None;  // Keep culling disabled for procedural meshes
    pipelineDesc.frontFace = FrontFace::CounterClockwise;
    pipelineDesc.depthTestEnabled = true;
    pipelineDesc.depthWriteEnabled = true;
    pipelineDesc.depthCompareOp = CompareOp::Less;

    // CRITICAL: Set render target and depth formats to match swapchain/depth buffer
    pipelineDesc.renderTargetFormats.push_back(Format::R8G8B8A8_UNORM);  // Swapchain format
    pipelineDesc.depthStencilFormat = Format::D32_FLOAT;  // Depth buffer format
    pipelineDesc.sampleCount = 1;
    pipelineDesc.debugName = "CreaturePipeline";

    g_app.creaturePipeline = g_app.device->CreatePipeline(pipelineDesc);
    if (!g_app.creaturePipeline) {
        std::cerr << "  ERROR: Failed to create creature pipeline!" << std::endl;
        return false;
    }
    std::cout << "  Pipeline created" << std::endl;

    // Initialize procedural creature mesh cache
    g_app.creatureMeshCache = std::make_unique<CreatureMeshCache>();
    g_app.creatureMeshCache->preloadArchetypes();
    g_app.creatureMeshDx12.reserve(MAX_CREATURE_MESHES_DX12);

    // Create large constant buffer for all creatures (256 bytes per creature)
    BufferDesc cbDesc;
    cbDesc.size = AppState::MAX_CB_CREATURES * 256;  // Room for 2048 creatures
    cbDesc.usage = BufferUsage::Uniform;
    cbDesc.cpuAccess = true;
    cbDesc.debugName = "CreatureConstantsCB";

    g_app.creatureConstantBuffer = g_app.device->CreateBuffer(cbDesc);
    if (!g_app.creatureConstantBuffer) {
        std::cerr << "  ERROR: Failed to create constant buffer!" << std::endl;
        return false;
    }
    void* testMap = g_app.creatureConstantBuffer->Map();
    if (!testMap) {
        std::cerr << "  ERROR: Failed to map creature constant buffer!" << std::endl;
        return false;
    }
    g_app.creatureConstantBuffer->Unmap();

    g_app.creatureRenderingInitialized = true;
    std::cout << "  Creature rendering initialized successfully!" << std::endl;
    std::cout << "  CreatureConstants size: " << sizeof(AppState::CreatureConstants) << " bytes" << std::endl;
    std::cout << "  Total CB size: " << cbDesc.size << " bytes for " << AppState::MAX_CB_CREATURES << " creatures" << std::endl;
    std::cout << "  Max instances: " << AppState::MAX_CREATURE_INSTANCES << std::endl;

    return true;
}

static CreatureMeshDX12* GetOrCreateCreatureMeshDX12(const Genome& genome, CreatureType type) {
    if (!g_app.device || !g_app.creatureMeshCache) {
        return nullptr;
    }

    CreatureType renderType = GetRenderBaseType(type);
    MeshKey key = CreatureMeshCache::getMeshKey(genome, renderType);
    auto it = g_app.creatureMeshDx12.find(key);
    if (it != g_app.creatureMeshDx12.end()) {
        return &it->second;
    }
    if (g_app.creatureMeshDx12.size() >= MAX_CREATURE_MESHES_DX12) {
        static bool warnedOnce = false;
        if (!warnedOnce) {
            std::cerr << "[CreatureMeshDX12] Cache full (" << MAX_CREATURE_MESHES_DX12
                      << "). Reusing existing mesh to avoid GPU buffer exhaustion." << std::endl;
            warnedOnce = true;
        }
        for (auto& entry : g_app.creatureMeshDx12) {
            if (entry.first.type == renderType) {
                return &entry.second;
            }
        }
        if (!g_app.creatureMeshDx12.empty()) {
            return &g_app.creatureMeshDx12.begin()->second;
        }
        return nullptr;
    }

    MeshData* mesh = g_app.creatureMeshCache->getMesh(genome, renderType);
    if (!mesh || mesh->vertices.empty() || mesh->indices.empty()) {
        return nullptr;
    }

    CreatureMeshDX12 dx12Mesh;
    dx12Mesh.vertexStride = sizeof(CreatureVertexDX12);
    dx12Mesh.vertexCount = static_cast<u32>(mesh->vertices.size());
    dx12Mesh.indexCount = static_cast<u32>(mesh->indices.size());

    glm::vec3 boundsMin = mesh->vertices[0].position;
    glm::vec3 boundsMax = mesh->vertices[0].position;
    for (const auto& v : mesh->vertices) {
        boundsMin = glm::min(boundsMin, v.position);
        boundsMax = glm::max(boundsMax, v.position);
    }
    dx12Mesh.boundsMin = boundsMin;
    dx12Mesh.boundsMax = boundsMax;

    std::vector<CreatureVertexDX12> vertices;
    vertices.reserve(mesh->vertices.size());
    for (const auto& v : mesh->vertices) {
        CreatureVertexDX12 dxv;
        dxv.position[0] = v.position.x;
        dxv.position[1] = v.position.y;
        dxv.position[2] = v.position.z;
        dxv.padding1 = 0.0f;
        dxv.normal[0] = v.normal.x;
        dxv.normal[1] = v.normal.y;
        dxv.normal[2] = v.normal.z;
        dxv.padding2 = 0.0f;
        dxv.texCoord[0] = v.texCoord.x;
        dxv.texCoord[1] = v.texCoord.y;
        vertices.push_back(dxv);
    }

    BufferDesc vbDesc;
    vbDesc.size = vertices.size() * sizeof(CreatureVertexDX12);
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuAccess = true;
    vbDesc.debugName = "CreatureMeshVB";

    dx12Mesh.vertexBuffer = g_app.device->CreateBuffer(vbDesc);
    if (!dx12Mesh.vertexBuffer) {
        return nullptr;
    }

    void* vbData = dx12Mesh.vertexBuffer->Map();
    if (!vbData) {
        return nullptr;
    }
    memcpy(vbData, vertices.data(), vbDesc.size);
    dx12Mesh.vertexBuffer->Unmap();

    BufferDesc ibDesc;
    ibDesc.size = mesh->indices.size() * sizeof(uint32_t);
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuAccess = true;
    ibDesc.debugName = "CreatureMeshIB";

    dx12Mesh.indexBuffer = g_app.device->CreateBuffer(ibDesc);
    if (!dx12Mesh.indexBuffer) {
        return nullptr;
    }

    void* ibData = dx12Mesh.indexBuffer->Map();
    if (!ibData) {
        return nullptr;
    }
    memcpy(ibData, mesh->indices.data(), ibDesc.size);
    dx12Mesh.indexBuffer->Unmap();

    g_app.creatureMeshDx12[key] = std::move(dx12Mesh);
    return &g_app.creatureMeshDx12[key];
}

// ============================================================================
// Creature Rendering - Render Function
// ============================================================================
void RenderCreatures(const glm::mat4& viewProj,
                     const glm::vec3& cameraPos,
                     const glm::vec3& lightDir,
                     const glm::vec3& lightColor) {
    if (!g_app.creatureRenderingInitialized) {
        static bool warnOnce = true;
        if (warnOnce) {
            std::cout << "[RenderCreatures] WARNING: Rendering not initialized!" << std::endl;
            warnOnce = false;
        }
        return;
    }
    if (!g_app.creatureConstantBuffer) {
        std::cerr << "[RenderCreatures] ERROR: Constant buffer not initialized!" << std::endl;
        return;
    }

    // Debug output (first frame only)
    static bool firstFrame = true;

    struct CreatureDrawItem {
        CreatureMeshDX12* mesh = nullptr;
        u32 cbOffset = 0;
    };

    std::vector<CreatureDrawItem> drawItems;
    const bool useUnified = g_app.useUnifiedSimulation && g_app.creatureManager;
    size_t estimatedCount = useUnified
        ? static_cast<size_t>(g_app.creatureManager->getTotalPopulation())
        : static_cast<size_t>(g_app.world.GetAliveCount());
    drawItems.reserve(std::min(estimatedCount, static_cast<size_t>(AppState::MAX_CB_CREATURES)));

    void* creatureCbData = g_app.creatureConstantBuffer->Map();
    if (!creatureCbData) {
        std::cerr << "[RenderCreatures] ERROR: Failed to map constant buffer! Buffer exists but Map() returned null." << std::endl;
        std::cerr << "  Buffer size: " << g_app.creatureConstantBuffer->GetSize() << std::endl;
        return;
    }

    size_t creatureIndex = 0;
    auto addCreature = [&](SimCreature* c) {
        if (!c || !c->alive || creatureIndex >= AppState::MAX_CB_CREATURES) return;

        CreatureMeshDX12* mesh = GetOrCreateCreatureMeshDX12(c->genome, c->type);
        if (!mesh || !mesh->isValid()) {
            return;
        }

        // Base scale factor (procedural meshes already encode genome size)
        float baseScale = 2.2f;

        float typeScaleMultiplier = 1.0f;
        CreatureType renderType = GetRenderBaseType(c->type);
        switch (renderType) {
            case CreatureType::HERBIVORE:
                typeScaleMultiplier = 1.0f;
                break;
            case CreatureType::CARNIVORE:
                typeScaleMultiplier = 1.2f;
                break;
            case CreatureType::AQUATIC:
                typeScaleMultiplier = 1.1f;
                break;
            case CreatureType::FLYING:
                typeScaleMultiplier = 0.8f;
                break;
            default:
                typeScaleMultiplier = 1.0f;
                break;
        }

        float energyScaleFactor = 0.75f + 0.25f * std::clamp(c->energy / 100.0f, 0.0f, 1.0f);
        float finalScale = baseScale * typeScaleMultiplier * energyScaleFactor;
        float speed = glm::length(c->velocity);

        glm::vec3 baseColor = c->genome.color;
        glm::vec3 typeTint = GetTypeTint(c->type);
        glm::vec3 finalColor = glm::mix(baseColor, typeTint, 0.35f);

        float energyBrightness = 0.6f + 0.4f * std::clamp(c->energy / 100.0f, 0.0f, 1.0f);
        finalColor *= energyBrightness;

        float groundOffset = -mesh->boundsMin.y;
        float surfaceHeight = SimulationWorld::GetSurfaceHeight(*c);

        AppState::CreatureConstants* cb = reinterpret_cast<AppState::CreatureConstants*>(
            static_cast<u8*>(creatureCbData) + creatureIndex * 256);

        memcpy(cb->viewProj, glm::value_ptr(viewProj), sizeof(float) * 16);
        cb->viewPos[0] = cameraPos.x;
        cb->viewPos[1] = cameraPos.y;
        cb->viewPos[2] = cameraPos.z;
        cb->viewPos[3] = 1.0f;

        cb->lightDir[0] = lightDir.x;
        cb->lightDir[1] = lightDir.y;
        cb->lightDir[2] = lightDir.z;
        cb->lightDir[3] = 0.0f;

        cb->lightColor[0] = lightColor.x;
        cb->lightColor[1] = lightColor.y;
        cb->lightColor[2] = lightColor.z;
        cb->lightColor[3] = 1.0f;

        cb->time = g_app.world.simulationTime;
        cb->padding[0] = cb->padding[1] = cb->padding[2] = 0.0f;

        cb->objectPos[0] = c->position.x;
        cb->objectPos[1] = surfaceHeight + groundOffset * finalScale + CREATURE_GROUND_CLEARANCE;
        cb->objectPos[2] = c->position.z;
        cb->objectPos[3] = 0.0f;

        float lengthFactor = 0.9f + std::clamp((c->genome.speed - 5.0f) / 15.0f, 0.0f, 1.0f) * 0.3f;
        float heightFactor = 0.9f + std::clamp((c->genome.efficiency - 0.5f) / 1.0f, 0.0f, 1.0f) * 0.2f;
        float widthFactor = 0.85f + std::clamp((c->genome.size - 0.5f) / 1.5f, 0.0f, 1.0f) * 0.3f;
        if (renderType == CreatureType::FLYING) {
            float wingSpan = std::clamp(c->genome.wingSpan, 0.5f, 2.0f);
            lengthFactor *= 0.8f + wingSpan * 0.3f;
            widthFactor *= 0.7f + wingSpan * 0.35f;
            heightFactor *= 0.85f;
        } else if (renderType == CreatureType::AQUATIC) {
            float tailSize = std::clamp(c->genome.tailSize, 0.5f, 1.2f);
            float finSize = std::clamp(c->genome.finSize, 0.3f, 1.0f);
            lengthFactor *= 0.85f + tailSize * 0.35f;
            widthFactor *= 0.8f + finSize * 0.3f;
            heightFactor *= 0.9f;
        }
        float phaseOffset = std::fmod(static_cast<float>(c->id) * 0.618f, 1.0f);
        cb->objectScale[0] = finalScale * widthFactor;
        cb->objectScale[1] = finalScale * heightFactor;
        cb->objectScale[2] = finalScale * lengthFactor;
        cb->objectScale[3] = phaseOffset;

        cb->objectColor[0] = finalColor.x;
        cb->objectColor[1] = finalColor.y;
        cb->objectColor[2] = finalColor.z;
        float typeTag = 0.0f;
        if (renderType == CreatureType::FLYING) {
            typeTag = 1.0f;
        } else if (renderType == CreatureType::AQUATIC) {
            typeTag = 2.0f;
        }
        cb->objectColor[3] = typeTag;

        glm::vec3 renderDir = c->facing;
        if (renderType != CreatureType::FLYING && renderType != CreatureType::AQUATIC) {
            renderDir.y = 0.0f;
        }
        cb->objectDir[0] = renderDir.x;
        cb->objectDir[1] = renderDir.y;
        cb->objectDir[2] = renderDir.z;
        cb->objectDir[3] = speed;

        CreatureDrawItem item;
        item.mesh = mesh;
        item.cbOffset = static_cast<u32>(creatureIndex * 256);
        drawItems.push_back(item);
        creatureIndex++;
    };

    auto addUnifiedCreature = [&](Creature* c) {
        if (!c || !c->isAlive() || creatureIndex >= AppState::MAX_CB_CREATURES) return;

        CreatureMeshDX12* mesh = GetOrCreateCreatureMeshDX12(c->getGenome(), c->getType());
        if (!mesh || !mesh->isValid()) {
            return;
        }

        float baseScale = 2.2f;
        CreatureType renderType = GetRenderBaseType(c->getType());
        float typeScaleMultiplier = 1.0f;
        switch (renderType) {
            case CreatureType::HERBIVORE:
                typeScaleMultiplier = 1.0f;
                break;
            case CreatureType::CARNIVORE:
                typeScaleMultiplier = 1.2f;
                break;
            case CreatureType::AQUATIC:
                typeScaleMultiplier = 1.1f;
                break;
            case CreatureType::FLYING:
                typeScaleMultiplier = 0.8f;
                break;
            default:
                typeScaleMultiplier = 1.0f;
                break;
        }

        float maxEnergy = std::max(1.0f, c->getMaxEnergy());
        float energyScaleFactor = 0.75f + 0.25f * std::clamp(c->getEnergy() / maxEnergy, 0.0f, 1.0f);
        float finalScale = baseScale * typeScaleMultiplier * energyScaleFactor;
        glm::vec3 velocity = c->getVelocity();
        float speed = glm::length(velocity);

        glm::vec3 baseColor = c->getSpeciesTintedColor();
        glm::vec3 typeTint = GetTypeTint(c->getType());
        glm::vec3 finalColor = glm::mix(baseColor, typeTint, 0.35f);
        float energyBrightness = 0.6f + 0.4f * std::clamp(c->getEnergy() / maxEnergy, 0.0f, 1.0f);
        finalColor *= energyBrightness;

        float groundOffset = -mesh->boundsMin.y;
        float surfaceHeight = GetCreatureRenderSurfaceHeight(*c);

        AppState::CreatureConstants* cb = reinterpret_cast<AppState::CreatureConstants*>(
            static_cast<u8*>(creatureCbData) + creatureIndex * 256);

        memcpy(cb->viewProj, glm::value_ptr(viewProj), sizeof(float) * 16);
        cb->viewPos[0] = cameraPos.x;
        cb->viewPos[1] = cameraPos.y;
        cb->viewPos[2] = cameraPos.z;
        cb->viewPos[3] = 1.0f;

        cb->lightDir[0] = lightDir.x;
        cb->lightDir[1] = lightDir.y;
        cb->lightDir[2] = lightDir.z;
        cb->lightDir[3] = 0.0f;

        cb->lightColor[0] = lightColor.x;
        cb->lightColor[1] = lightColor.y;
        cb->lightColor[2] = lightColor.z;
        cb->lightColor[3] = 1.0f;

        cb->time = g_app.world.simulationTime;
        cb->padding[0] = cb->padding[1] = cb->padding[2] = 0.0f;

        cb->objectPos[0] = c->getPosition().x;
        cb->objectPos[1] = surfaceHeight + groundOffset * finalScale + CREATURE_GROUND_CLEARANCE;
        cb->objectPos[2] = c->getPosition().z;
        cb->objectPos[3] = 0.0f;

        const Genome& genome = c->getGenome();
        float lengthFactor = 0.9f + std::clamp((genome.speed - 5.0f) / 15.0f, 0.0f, 1.0f) * 0.3f;
        float heightFactor = 0.9f + std::clamp((genome.efficiency - 0.5f) / 1.0f, 0.0f, 1.0f) * 0.2f;
        float widthFactor = 0.85f + std::clamp((genome.size - 0.5f) / 1.5f, 0.0f, 1.0f) * 0.3f;
        if (renderType == CreatureType::FLYING) {
            float wingSpan = std::clamp(genome.wingSpan, 0.5f, 2.0f);
            lengthFactor *= 0.8f + wingSpan * 0.3f;
            widthFactor *= 0.7f + wingSpan * 0.35f;
            heightFactor *= 0.85f;
        } else if (renderType == CreatureType::AQUATIC) {
            float tailSize = std::clamp(genome.tailSize, 0.5f, 1.2f);
            float finSize = std::clamp(genome.finSize, 0.3f, 1.0f);
            lengthFactor *= 0.85f + tailSize * 0.35f;
            widthFactor *= 0.8f + finSize * 0.3f;
            heightFactor *= 0.9f;
        }
        float phaseOffset = std::fmod(static_cast<float>(c->getID()) * 0.618f, 1.0f);
        cb->objectScale[0] = finalScale * widthFactor;
        cb->objectScale[1] = finalScale * heightFactor;
        cb->objectScale[2] = finalScale * lengthFactor;
        cb->objectScale[3] = phaseOffset;

        cb->objectColor[0] = finalColor.x;
        cb->objectColor[1] = finalColor.y;
        cb->objectColor[2] = finalColor.z;
        float typeTag = 0.0f;
        if (renderType == CreatureType::FLYING) {
            typeTag = 1.0f;
        } else if (renderType == CreatureType::AQUATIC) {
            typeTag = 2.0f;
        }
        cb->objectColor[3] = typeTag;

        glm::vec3 renderDir = velocity;
        if (glm::length(renderDir) < 0.01f) {
            renderDir = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        if (renderType != CreatureType::FLYING && renderType != CreatureType::AQUATIC) {
            renderDir.y = 0.0f;
        }
        cb->objectDir[0] = renderDir.x;
        cb->objectDir[1] = renderDir.y;
        cb->objectDir[2] = renderDir.z;
        cb->objectDir[3] = speed;

        CreatureDrawItem item;
        item.mesh = mesh;
        item.cbOffset = static_cast<u32>(creatureIndex * 256);
        drawItems.push_back(item);
        creatureIndex++;
    };

    // During replay, render from replay creatures; otherwise render live creatures
    if (useUnified) {
        g_app.creatureManager->forEach([&](Creature& creature, size_t) {
            addUnifiedCreature(&creature);
        });
    } else if (g_app.isPlayingReplay) {
        for (auto& c : g_app.replayCreatures) {
            addCreature(&c);
        }
    } else if (g_app.world.usePooling) {
        for (SimCreature* c : g_app.world.activeCreatures) {
            addCreature(c);
        }
    } else {
        for (const auto& c : g_app.world.creatures) {
            addCreature(c.get());
        }
    }

    g_app.creatureConstantBuffer->Unmap();

    if (drawItems.empty()) {
        if (firstFrame) {
            std::cout << "[RenderCreatures] WARNING: No instances to render!" << std::endl;
            firstFrame = false;
        }
        return;
    }

    if (firstFrame) {
        std::cout << "[RenderCreatures] Rendering " << drawItems.size() << " creatures" << std::endl;
        std::cout << "  Camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
        std::cout << "  Camera target: " << g_app.cameraTarget.x << ", " << g_app.cameraTarget.y << ", " << g_app.cameraTarget.z << std::endl;
        std::cout << "  First creature at: " << drawItems[0].cbOffset << " (cb offset)" << std::endl;
        // Print viewProj matrix - glm is column-major so [col][row]
        // For a perspective matrix looking at origin from above, we should see non-trivial values
        std::cout << "  ViewProj diagonal: " << viewProj[0][0] << ", " << viewProj[1][1] << ", "
                  << viewProj[2][2] << ", " << viewProj[3][3] << std::endl;
        std::cout << "  ViewProj[3] (translation col): " << viewProj[3][0] << ", " << viewProj[3][1] << ", "
                  << viewProj[3][2] << ", " << viewProj[3][3] << std::endl;
        firstFrame = false;
    }

    // Set up rendering state
    g_app.commandList->SetPipeline(g_app.creaturePipeline.get());

    for (const auto& item : drawItems) {
        if (!item.mesh || !item.mesh->isValid()) {
            continue;
        }

        g_app.commandList->BindVertexBuffer(0, item.mesh->vertexBuffer.get(), item.mesh->vertexStride, 0);
        g_app.commandList->BindIndexBuffer(item.mesh->indexBuffer.get(), IndexFormat::UInt32, 0);
        g_app.commandList->BindConstantBuffer(0, g_app.creatureConstantBuffer.get(), item.cbOffset);
        g_app.commandList->DrawIndexed(item.mesh->indexCount, 0, 0);
    }
}

void RenderCreatureNametags(const glm::mat4& viewProj, const glm::vec3& cameraPos) {
    if (!g_app.showNametags) return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    float maxDistSq = g_app.nametagMaxDistance * g_app.nametagMaxDistance;
    const bool useUnified = g_app.useUnifiedSimulation && g_app.creatureManager;

    auto drawCreature = [&](SimCreature* c) {
        if (!c || !c->alive) return;

        glm::vec3 offsetPos = c->position;
        offsetPos.y += 2.5f + c->genome.size;

        glm::vec4 clip = viewProj * glm::vec4(offsetPos, 1.0f);
        if (clip.w <= 0.001f) return;

        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) {
            return;
        }

        glm::vec3 toCamera = c->position - cameraPos;
        if (glm::dot(toCamera, toCamera) > maxDistSq) {
            return;
        }

        float screenX = (ndcX * 0.5f + 0.5f) * io.DisplaySize.x;
        float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * io.DisplaySize.y;

        glm::vec3 tint = GetTypeTint(c->type);
        ImU32 color = IM_COL32(static_cast<int>(tint.r * 255.0f),
                               static_cast<int>(tint.g * 255.0f),
                               static_cast<int>(tint.b * 255.0f), 230);

        char label[64];
        std::snprintf(label, sizeof(label), "%s #%u E:%.0f",
                      getCreatureTypeName(c->type), c->id, c->energy);

        drawList->AddText(ImVec2(screenX, screenY), color, label);
    };

    auto drawUnifiedCreature = [&](const Creature* c) {
        if (!c || !c->isAlive()) return;

        glm::vec3 offsetPos = c->getPosition();
        offsetPos.y += 2.5f + c->getGenome().size;

        glm::vec4 clip = viewProj * glm::vec4(offsetPos, 1.0f);
        if (clip.w <= 0.001f) return;

        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) {
            return;
        }

        glm::vec3 toCamera = offsetPos - cameraPos;
        if (glm::dot(toCamera, toCamera) > maxDistSq) {
            return;
        }

        float screenX = (ndcX * 0.5f + 0.5f) * io.DisplaySize.x;
        float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * io.DisplaySize.y;

        glm::vec3 tint = GetTypeTint(c->getType());
        ImU32 color = IM_COL32(static_cast<int>(tint.r * 255.0f),
                               static_cast<int>(tint.g * 255.0f),
                               static_cast<int>(tint.b * 255.0f), 230);

        char label[64];
        std::snprintf(label, sizeof(label), "%s #%d E:%.0f",
                      getCreatureTypeName(c->getType()), c->getID(), c->getEnergy());

        drawList->AddText(ImVec2(screenX, screenY), color, label);
    };

    if (useUnified) {
        g_app.creatureManager->forEach([&](Creature& creature, size_t) {
            drawUnifiedCreature(&creature);
        });
    } else if (g_app.world.usePooling) {
        for (SimCreature* c : g_app.world.activeCreatures) {
            drawCreature(c);
        }
    } else {
        for (const auto& c : g_app.world.creatures) {
            drawCreature(c.get());
        }
    }
}

// ============================================================================
// Loading Screen (Phase 5 - Polish)
// ============================================================================
void RenderLoadingScreen() {
    if (!g_app.isLoading) return;

    ImGuiIO& io = ImGui::GetIO();
    float width = 400.0f;
    float height = 100.0f;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2 - width / 2,
                                    io.DisplaySize.y / 2 - height / 2));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::SetNextWindowBgAlpha(0.9f);

    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Text("%s", g_app.loadingStatus.c_str());
    ImGui::Spacing();
    ImGui::Text("Progress: %.0f%%", g_app.loadingProgress * 100.0f);
    ImGui::ProgressBar(g_app.loadingProgress, ImVec2(-1, 0));

    ImGui::End();
}

// ============================================================================
// Help Overlay (Phase 5 - UX)
// ============================================================================
void RenderHelpOverlay() {
    if (!g_app.showHelpOverlay) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350));

    if (ImGui::Begin("Help - Keyboard Shortcuts", &g_app.showHelpOverlay)) {
        ImGui::Text("Camera Controls:");
        ImGui::Separator();
        ImGui::BulletText("Left Click - Capture/Release mouse (FPS look)");
        ImGui::BulletText("WASD - Move camera (FPS) / Move target (orbit)");
        ImGui::BulletText("Right Mouse + Drag - Rotate camera (orbit)");
        ImGui::BulletText("Mouse Scroll - Zoom in/out (orbit)");
        ImGui::BulletText("R - Reset camera to default position");

        ImGui::Spacing();
        ImGui::Text("Simulation Controls:");
        ImGui::Separator();
        ImGui::BulletText("P - Pause/Resume simulation");
        ImGui::BulletText("Space - Pause/Resume (in replay mode)");

        ImGui::Spacing();
        ImGui::Text("Save/Load:");
        ImGui::Separator();
        ImGui::BulletText("F5 - Quick Save");
        ImGui::BulletText("F9 - Quick Load");

        ImGui::Spacing();
        ImGui::Text("Replay:");
        ImGui::Separator();
        ImGui::BulletText("F10 - Toggle Replay Mode");
        ImGui::BulletText("Left/Right Arrow - Step frames");

        ImGui::Spacing();
        ImGui::Text("UI Panels:");
        ImGui::Separator();
        ImGui::BulletText("F1 - Toggle Debug Panel");
        ImGui::BulletText("F2 - Toggle Performance Profiler");
        ImGui::BulletText("F3 - Toggle Help (this window)");

        ImGui::Spacing();
        ImGui::Text("Camera (New):");
        ImGui::Separator();
        ImGui::BulletText("F - Follow selected creature");
        ImGui::BulletText("1/2/3 - Camera presets");
        ImGui::BulletText("Scroll - Zoom");

        ImGui::Spacing();
        ImGui::BulletText("ESC - Exit (or release mouse if captured)");
    }
    ImGui::End();
}

// Include camera panels (Phase 6 - Camera & Selection)
#include "ui/CameraPanels.inl"

// ============================================================================
// Save/Load System Functions (Phase 4 integration)
// ============================================================================

bool SaveGame(const std::string& filename) {
    try {
        // Prepare header with ACTUAL values (not placeholders)
        Forge::SaveFileHeader header;
        header.timestamp = static_cast<uint64_t>(std::time(nullptr));
        header.creatureCount = g_app.world.GetAliveCount();
        header.foodCount = static_cast<uint32_t>(g_app.world.foods.size());
        header.generation = g_app.world.maxGeneration;  // Actual max generation
        header.simulationTime = g_app.world.simulationTime;
        header.terrainSeed = g_app.world.terrainSeed;   // Actual terrain seed

        // Prepare world data - save all critical state for deterministic continuation
        Forge::WorldSaveData worldData;
        worldData.terrainSeed = g_app.world.terrainSeed;
        worldData.dayTime = g_app.dayNight.GetTimeOfDay();
        worldData.dayDuration = g_app.dayNight.dayLengthSeconds;
        worldData.rngState = g_app.world.getRngState();        // Full RNG state for determinism
        worldData.maxGeneration = g_app.world.maxGeneration;   // Track evolution progress
        worldData.nextCreatureId = g_app.world.nextCreatureId; // Prevent ID collisions

        // Prepare creature data
        std::vector<Forge::CreatureSaveData> creatures;

        auto saveCreature = [&creatures](const SimCreature* c) {
            if (!c->alive) return;

            Forge::CreatureSaveData data;
            data.id = c->id;
            data.type = static_cast<uint8_t>(c->type);

            // Position and velocity
            data.posX = c->position.x;
            data.posY = c->position.y;
            data.posZ = c->position.z;
            data.velX = c->velocity.x;
            data.velY = c->velocity.y;
            data.velZ = c->velocity.z;
            data.rotation = std::atan2(c->facing.x, c->facing.z);

            // State - use actual creature values
            data.energy = c->energy;
            data.health = 100.0f;  // SimCreature doesn't track health separately
            data.age = 0.0f;       // SimCreature doesn't track age
            data.generation = 1;   // SimCreature doesn't track generation per-creature

            // Behavior tracking (SimCreature doesn't track these, but save zeros for format compatibility)
            data.foodEaten = 0.0f;
            data.distanceTraveled = 0.0f;
            data.successfulHunts = 0;
            data.escapes = 0;
            data.wanderAngle = 0.0f;
            data.animPhase = 0.0f;

            // Genome data - save all important genetic traits
            data.genomeSize = c->genome.size;
            data.genomeSpeed = c->genome.speed;
            data.genomeVision = c->genome.visionRange;
            data.genomeEfficiency = c->genome.efficiency;
            data.genomeColorR = c->genome.color.r;
            data.genomeColorG = c->genome.color.g;
            data.genomeColorB = c->genome.color.b;
            data.genomeMutationRate = 0.1f;

            // Neural network weights - store ALL genome neural weights
            // The genome stores weights as a single flattened vector
            data.weightsIH = c->genome.neuralWeights;
            // Note: SimCreature's Genome uses a single neuralWeights vector for all weights
            // We store it in weightsIH and leave others empty for this architecture
            data.weightsHO.clear();
            data.biasH.clear();
            data.biasO.clear();

            creatures.push_back(data);
        };

        if (g_app.world.usePooling) {
            for (SimCreature* c : g_app.world.activeCreatures) {
                saveCreature(c);
            }
        } else {
            for (const auto& c : g_app.world.creatures) {
                saveCreature(c.get());
            }
        }

        // Prepare food data
        std::vector<Forge::FoodSaveData> food;
        for (const auto& f : g_app.world.foods) {
            Forge::FoodSaveData data;
            data.posX = f->position.x;
            data.posY = f->position.y;
            data.posZ = f->position.z;
            data.energy = f->amount;
            data.respawnTimer = 0.0f;
            data.active = true;
            food.push_back(data);
        }

        // Save to file
        auto result = g_app.saveManager.saveGame(filename, header, worldData, creatures, food);
        if (result == Forge::SaveResult::Success) {
            g_app.statusMessage = "Game saved: " + filename;
            g_app.statusMessageTimer = 3.0f;
            return true;
        } else {
            g_app.statusMessage = "Save failed: " + g_app.saveManager.getLastError();
            g_app.statusMessageTimer = 3.0f;
            return false;
        }
    } catch (const std::exception& e) {
        g_app.statusMessage = "Save error: " + std::string(e.what());
        g_app.statusMessageTimer = 3.0f;
        return false;
    }
}

bool LoadGame(const std::string& filename) {
    try {
        Forge::SaveFileHeader header;
        Forge::WorldSaveData worldData;
        std::vector<Forge::CreatureSaveData> creatures;
        std::vector<Forge::FoodSaveData> food;

        auto result = g_app.saveManager.loadGame(filename, header, worldData, creatures, food);
        if (result != Forge::LoadResult::Success) {
            g_app.statusMessage = "Load failed: " + g_app.saveManager.getLastError();
            g_app.statusMessageTimer = 3.0f;
            return false;
        }

        // Clear existing state
        if (g_app.world.usePooling) {
            g_app.world.creaturePool.releaseDeadCreatures();
            // Release all remaining creatures
            for (SimCreature* c : g_app.world.activeCreatures) {
                if (c) c->alive = false;
            }
            g_app.world.creaturePool.releaseDeadCreatures();
            g_app.world.activeCreatures.clear();
        } else {
            g_app.world.creatures.clear();
        }
        g_app.world.foods.clear();

        // Restore simulation time
        g_app.world.simulationTime = header.simulationTime;

        // Restore terrain seed (for potential terrain regeneration)
        g_app.world.terrainSeed = worldData.terrainSeed;

        // Restore generation counter
        g_app.world.maxGeneration = worldData.maxGeneration;

        // CRITICAL: Restore nextCreatureId to prevent ID collisions
        // If worldData has it (v2+), use it; otherwise calculate from loaded creatures
        if (worldData.nextCreatureId > 0) {
            g_app.world.nextCreatureId = worldData.nextCreatureId;
        } else {
            // V1 save: calculate from max ID in loaded creatures + 1
            u32 maxId = 0;
            for (const auto& data : creatures) {
                if (data.id > maxId) maxId = data.id;
            }
            g_app.world.nextCreatureId = maxId + 1;
        }

        // Restore RNG state for deterministic continuation
        if (!worldData.rngState.empty()) {
            g_app.world.setRngState(worldData.rngState);
        }

        // Restore day/night cycle state
        g_app.dayNight.SetTimeOfDay(worldData.dayTime);
        if (worldData.dayDuration > 0.0f) {
            g_app.dayNight.dayLengthSeconds = worldData.dayDuration;
        }

        // Restore creatures with full genome data
        // NOTE: We temporarily disable pool ID assignment during load to preserve saved IDs
        for (const auto& data : creatures) {
            glm::vec3 pos(data.posX, data.posY, data.posZ);
            CreatureType type = static_cast<CreatureType>(data.type);

            SimCreature* c = g_app.world.SpawnCreature(pos, type, data.energy);
            if (c) {
                // Override the auto-assigned ID with the saved ID
                c->id = data.id;
                c->velocity = glm::vec3(data.velX, data.velY, data.velZ);

                // Restore facing direction from rotation
                c->facing = glm::vec3(std::sin(data.rotation), 0.0f, std::cos(data.rotation));

                // Restore genome traits
                c->genome.size = data.genomeSize;
                c->genome.speed = data.genomeSpeed;
                c->genome.visionRange = data.genomeVision;
                c->genome.efficiency = data.genomeEfficiency;
                c->genome.color = glm::vec3(data.genomeColorR, data.genomeColorG, data.genomeColorB);

                // Restore neural network weights if saved
                if (!data.weightsIH.empty()) {
                    c->genome.neuralWeights = data.weightsIH;
                }
            }
        }

        // After loading all creatures, ensure nextCreatureId is higher than all loaded IDs
        // This is a safety check in case SpawnCreature incremented it during load
        u32 maxLoadedId = 0;
        if (g_app.world.usePooling) {
            for (SimCreature* c : g_app.world.activeCreatures) {
                if (c && c->id > maxLoadedId) maxLoadedId = c->id;
            }
        } else {
            for (const auto& c : g_app.world.creatures) {
                if (c && c->id > maxLoadedId) maxLoadedId = c->id;
            }
        }
        if (g_app.world.nextCreatureId <= maxLoadedId) {
            g_app.world.nextCreatureId = maxLoadedId + 1;
        }

        // CRITICAL: Also update the pool's internal ID counter to prevent collisions
        if (g_app.world.usePooling) {
            g_app.world.creaturePool.setNextId(g_app.world.nextCreatureId);
        }

        // Restore food
        for (const auto& data : food) {
            if (data.active) {
                glm::vec3 pos(data.posX, data.posY, data.posZ);
                g_app.world.foods.push_back(std::make_unique<Food>(pos, data.energy));
            }
        }

        g_app.world.UpdateStats();

        g_app.statusMessage = "Game loaded: " + filename + " (gen " + std::to_string(worldData.maxGeneration) + ")";
        g_app.statusMessageTimer = 3.0f;
        return true;

    } catch (const std::exception& e) {
        g_app.statusMessage = "Load error: " + std::string(e.what());
        g_app.statusMessageTimer = 3.0f;
        return false;
    }
}

void QuickSave() {
    SaveGame("quicksave.evos");
}

void QuickLoad() {
    LoadGame("quicksave.evos");
}

// ============================================================================
// Replay System Functions (S-05, S-06 integration)
// ============================================================================

Forge::ReplayFrame BuildReplayFrame() {
    Forge::ReplayFrame frame;
    frame.timestamp = g_app.world.simulationTime;

    // Helper lambda to capture creature snapshot
    auto captureCreature = [&frame](const SimCreature* creature) {
        if (!creature->alive) return;

        Forge::CreatureSnapshot snap;
        snap.id = creature->id;
        snap.type = static_cast<uint8_t>(creature->type);
        snap.posX = creature->position.x;
        snap.posY = creature->position.y;
        snap.posZ = creature->position.z;
        snap.rotation = std::atan2(creature->velocity.x, creature->velocity.z);
        snap.health = 100.0f;
        snap.energy = creature->energy;
        snap.animPhase = 0.0f;

        // Capture actual visual properties from genome
        snap.colorR = creature->genome.color.r;
        snap.colorG = creature->genome.color.g;
        snap.colorB = creature->genome.color.b;
        snap.size = creature->genome.size;

        // Capture actual genome data
        snap.genomeSpeed = creature->genome.speed;
        snap.genomeSize = creature->genome.size;
        snap.genomeVision = creature->genome.visionRange;

        frame.creatures.push_back(snap);
    };

    // Capture creatures (S-07: neural weights support is in ReplaySystem.h)
    if (g_app.world.usePooling) {
        // Use pooled creatures
        for (SimCreature* creature : g_app.world.activeCreatures) {
            captureCreature(creature);
        }
    } else {
        // Legacy mode
        for (const auto& creature : g_app.world.creatures) {
            captureCreature(creature.get());
        }
    }

    // Capture food
    for (const auto& food : g_app.world.foods) {
        Forge::FoodSnapshot fsnap;
        fsnap.posX = food->position.x;
        fsnap.posY = food->position.y;
        fsnap.posZ = food->position.z;
        fsnap.energy = food->amount;
        fsnap.active = true;
        frame.food.push_back(fsnap);
    }

    // Capture camera
    frame.camera.posX = g_app.cameraPosition.x;
    frame.camera.posY = g_app.cameraPosition.y;
    frame.camera.posZ = g_app.cameraPosition.z;
    frame.camera.targetX = g_app.cameraTarget.x;
    frame.camera.targetY = g_app.cameraTarget.y;
    frame.camera.targetZ = g_app.cameraTarget.z;
    frame.camera.fov = 45.0f;

    // Capture statistics
    frame.stats.herbivoreCount = g_app.world.herbivoreCount;
    frame.stats.carnivoreCount = g_app.world.carnivoreCount;
    frame.stats.foodCount = static_cast<uint32_t>(g_app.world.foods.size());
    frame.stats.generation = 1;
    frame.stats.avgHerbivoreFitness = 0.0f;
    frame.stats.avgCarnivoreFitness = 0.0f;

    return frame;
}

void EnterReplayMode() {
    if (g_app.replayRecorder.getFrameCount() == 0) {
        g_app.statusMessage = "No replay data to play";
        g_app.statusMessageTimer = 3.0f;
        return;
    }

    g_app.isPlayingReplay = true;
    g_app.isRecording = false;
    g_app.world.paused = true;

    // Transfer frames from recorder to player
    g_app.replayPlayer.loadFromRecorder(g_app.replayRecorder);
    g_app.replayPlayer.play();

    g_app.statusMessage = "Entered replay mode - Press F10 to exit";
    g_app.statusMessageTimer = 3.0f;
}

void ExitReplayMode() {
    g_app.isPlayingReplay = false;
    g_app.isRecording = true;
    g_app.replayPlayer.stop();

    g_app.statusMessage = "Exited replay mode - Recording resumed";
    g_app.statusMessageTimer = 3.0f;
}

void ApplyReplayFrame(const Forge::ReplayFrame& frame) {
    // Store current frame for rendering
    g_app.currentReplayFrame = frame;

    // Update camera from replay frame
    g_app.cameraTarget.x = frame.camera.targetX;
    g_app.cameraTarget.y = frame.camera.targetY;
    g_app.cameraTarget.z = frame.camera.targetZ;

    // Calculate camera angles from position
    glm::vec3 replayPos(frame.camera.posX, frame.camera.posY, frame.camera.posZ);
    glm::vec3 toCamera = replayPos - g_app.cameraTarget;
    g_app.cameraDistance = glm::length(toCamera);
    if (g_app.cameraDistance > 0.1f) {
        g_app.cameraPitch = glm::degrees(std::asin(toCamera.y / g_app.cameraDistance));
        g_app.cameraYaw = glm::degrees(std::atan2(toCamera.x, toCamera.z));
    }

    // Populate replay creatures for rendering
    g_app.replayCreatures.clear();
    g_app.replayCreatures.reserve(frame.creatures.size());

    for (const auto& snap : frame.creatures) {
        SimCreature creature;
        creature.id = snap.id;
        creature.position = glm::vec3(snap.posX, snap.posY, snap.posZ);
        creature.velocity = glm::vec3(std::sin(snap.rotation), 0.0f, std::cos(snap.rotation)) * 0.1f;
        creature.facing = glm::normalize(glm::vec3(std::sin(snap.rotation), 0.0f, std::cos(snap.rotation)));
        creature.energy = snap.energy;
        creature.type = static_cast<CreatureType>(snap.type);
        creature.alive = true;
        creature.fear = 0.0f;
        creature.pooled = false;

        // Set genome from snapshot data
        creature.genome.speed = snap.genomeSpeed;
        creature.genome.size = snap.genomeSize;
        creature.genome.visionRange = snap.genomeVision;
        creature.genome.color = glm::vec3(snap.colorR, snap.colorG, snap.colorB);

        g_app.replayCreatures.push_back(creature);
    }

    // Update statistics display from replay
    g_app.world.herbivoreCount = frame.stats.herbivoreCount;
    g_app.world.carnivoreCount = frame.stats.carnivoreCount;
}

// ============================================================================
// ImGui Initialization
// ============================================================================
bool InitializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (io.Fonts && io.Fonts->Fonts.Size == 0) {
        io.Fonts->AddFontDefault();
    }

    ImGui::StyleColorsDark();

    // Create SRV descriptor heap for ImGui
    ID3D12Device* d3dDevice = static_cast<ID3D12Device*>(g_app.device->GetNativeDevice());

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = MAX_SRV_DESCRIPTORS;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_app.imguiSrvHeap));
    if (FAILED(hr)) {
        std::cerr << "Failed to create ImGui descriptor heap" << std::endl;
        return false;
    }

    // Initialize ImGui Win32 platform
    HWND hwnd = static_cast<HWND>(g_app.window->GetNativeHandle());
    if (!ImGui_ImplWin32_Init(hwnd)) {
        std::cerr << "Failed to initialize ImGui Win32 backend" << std::endl;
        ImGui::DestroyContext();
        return false;
    }

    // Initialize ImGui DX12 renderer
    if (!ImGui_ImplDX12_Init(
        d3dDevice,
        FRAME_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        g_app.imguiSrvHeap.Get(),
        g_app.imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_app.imguiSrvHeap->GetGPUDescriptorHandleForHeapStart()
    )) {
        std::cerr << "Failed to initialize ImGui DX12 backend" << std::endl;
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        std::cerr << "Failed to create ImGui DX12 device objects" << std::endl;
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    g_app.imguiInitialized = true;

    // Phase 10 Agent 8: Setup selection system callback
    g_app.selectionSystem.setOnSelectionChanged([](const ui::SelectionChangedEvent& event) {
        if (event.newSelection) {
            g_app.inspectionPanel.setInspectedCreature(event.newSelection);
        } else if (event.wasCleared) {
            g_app.inspectionPanel.clearInspection();
        }
    });

    g_app.inspectionPanel.setFocusCameraCallback([](const Creature* creature) {
        if (!creature) {
            return;
        }
        glm::vec3 forward = creature->getVelocity();
        if (glm::length(forward) < 0.01f) {
            forward = glm::vec3(0.0f, 0.0f, 1.0f);
        } else {
            forward = glm::normalize(forward);
        }
        glm::vec3 target = creature->getPosition() + glm::vec3(0.0f, 2.0f, 0.0f);
        glm::vec3 camPos = target - forward * 25.0f + glm::vec3(0.0f, 10.0f, 0.0f);
        StartCameraTransition(camPos, target, 1.2f);
        g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
        g_app.followCreature = nullptr;
        g_app.followCreatureId = -1;
    });

    g_app.inspectionPanel.setTrackCameraCallback([](const Creature* creature) {
        if (!creature) {
            return;
        }
        g_app.cameraFollowMode = AppState::CameraFollowMode::FOLLOW;
        g_app.followCreature = creature;
        g_app.followCreatureId = -1;
        g_app.followOrbitAngle = g_app.cameraYaw;
    });

    g_app.inspectionPanel.setReleaseCameraCallback([]() {
        g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
        g_app.followCreature = nullptr;
        g_app.followCreatureId = -1;
    });

    g_app.mainMenu.setOnStartGame([](const ui::WorldGenConfig& worldConfig,
                                     const ui::EvolutionStartPreset& evolutionPreset,
                                     bool godMode) {
        StartWorldGeneration(worldConfig, evolutionPreset, godMode);
    });

    g_app.mainMenu.setOnContinue([]() {
        g_app.world.paused = false;
    });

    g_app.mainMenu.setOnSettingsChanged([](const ui::SettingsConfig& settings) {
        ApplySettingsConfig(settings);
    });

    g_app.mainMenu.setOnQuit([]() {
        if (g_app.window) {
            g_app.window->Close();
        }
    });

    return true;
}

// ============================================================================
// GPU Steering Initialization
// ============================================================================
bool InitializeGPUSteering() {
    ID3D12Device* d3dDevice = static_cast<ID3D12Device*>(g_app.device->GetNativeDevice());

    g_app.dx12DeviceAdapter.SetDevice(d3dDevice);

    if (!g_app.dx12DeviceAdapter.IsValid()) {
        std::cerr << "Failed to set up DX12 device adapter" << std::endl;
        return false;
    }

    g_app.gpuSteering = std::make_unique<GPUSteeringCompute>();

    if (!g_app.gpuSteering->Initialize(&g_app.dx12DeviceAdapter)) {
        std::cerr << "Failed to initialize GPU steering compute" << std::endl;
        g_app.gpuSteering.reset();
        return false;
    }

    g_app.gpuSteeringAvailable = true;
    g_app.gpuSteeringEnabled = true;

    std::cout << "GPU Steering Compute initialized successfully!" << std::endl;
    std::cout << "  Max creatures: " << GPUSteeringCompute::MAX_CREATURES << std::endl;
    std::cout << "  Thread group size: " << GPUSteeringCompute::THREAD_GROUP_SIZE << std::endl;

    return true;
}

// ============================================================================
// Dispatch GPU Steering
// ============================================================================
void DispatchGPUSteering(std::vector<SteeringOutput>& results) {
    if (g_app.useUnifiedSimulation) {
        return;
    }
    if (!g_app.gpuSteeringEnabled || !g_app.gpuSteering || !g_app.gpuSteering->IsInitialized()) {
        return;
    }

    u32 creatureCount = g_app.world.GetAliveCount();
    if (creatureCount < GPU_STEERING_THRESHOLD) {
        return;  // Use CPU for small populations
    }

    if (!g_app.computeCommandList || !g_app.computeFence) {
        std::cerr << "GPU steering skipped: compute command list or fence not initialized" << std::endl;
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Prepare creature input data
    std::vector<CreatureInput> creatureInputs;
    creatureInputs.reserve(creatureCount);

    // Helper lambda to add creature input
    auto addCreatureInput = [&creatureInputs](const SimCreature* creature) {
        if (!creature->alive) return;

        CreatureInput ci = {};
        ci.position = XMFLOAT3(creature->position.x, creature->position.y, creature->position.z);
        ci.velocity = XMFLOAT3(creature->velocity.x, creature->velocity.y, creature->velocity.z);
        ci.energy = creature->energy;
        ci.fear = creature->fear;
        ci.type = static_cast<uint32_t>(creature->type);
        ci.isAlive = creature->alive ? 1 : 0;
        ci.waterLevel = 0.0f;  // TODO: Get from terrain

        creatureInputs.push_back(ci);
    };

    // Use pooled or legacy creatures
    if (g_app.world.usePooling) {
        g_app.world.activeCreatures = g_app.world.creaturePool.getActiveCreatures();
        for (SimCreature* creature : g_app.world.activeCreatures) {
            addCreatureInput(creature);
        }
    } else {
        for (const auto& creature : g_app.world.creatures) {
            addCreatureInput(creature.get());
        }
    }

    // Prepare food data
    std::vector<FoodPosition> foodPositions;
    foodPositions.reserve(g_app.world.foods.size());

    for (const auto& food : g_app.world.foods) {
        FoodPosition fp = {};
        fp.position = XMFLOAT3(food->position.x, food->position.y, food->position.z);
        fp.amount = food->amount;
        foodPositions.push_back(fp);
    }

    // Update GPU buffers
    g_app.gpuSteering->UpdateCreatureData(creatureInputs);
    g_app.gpuSteering->UpdateFoodData(foodPositions);

    // Update constants
    SteeringConstants constants;
    constants.creatureCount = static_cast<uint32_t>(creatureInputs.size());
    constants.foodCount = static_cast<uint32_t>(foodPositions.size());
    constants.deltaTime = g_app.deltaTime;
    constants.time = g_app.world.simulationTime;
    g_app.gpuSteering->UpdateConstants(constants);

    if (constants.creatureCount == 0) {
        return;
    }

    // Record compute work on a dedicated command list
    auto dispatchStart = std::chrono::high_resolution_clock::now();
    g_app.computeCommandList->Begin();
    ID3D12GraphicsCommandList* cmdList = static_cast<ID3D12GraphicsCommandList*>(
        g_app.computeCommandList->GetNativeCommandList()
    );

    // Dispatch compute shader
    g_app.gpuSteering->Dispatch(cmdList, constants.creatureCount, g_app.deltaTime, g_app.world.simulationTime);
    g_app.gpuSteering->CopyOutputToReadback(cmdList, constants.creatureCount);

    g_app.computeCommandList->End();
    g_app.device->Submit(g_app.computeCommandList.get());

    g_app.computeFenceValue++;
    g_app.device->SignalFence(g_app.computeFence.get(), g_app.computeFenceValue);
    auto dispatchEnd = std::chrono::high_resolution_clock::now();

    g_app.gpuSteeringDispatchCount++;

    auto readbackStart = std::chrono::high_resolution_clock::now();
    g_app.device->WaitFence(g_app.computeFence.get(), g_app.computeFenceValue);

    // Read back results after GPU completes the copy
    results.resize(constants.creatureCount);
    g_app.gpuSteering->ReadbackResults(results, constants.creatureCount);
    auto readbackEnd = std::chrono::high_resolution_clock::now();

    g_app.timings.gpuSteeringDispatch =
        std::chrono::duration<float>(dispatchEnd - dispatchStart).count();
    g_app.timings.gpuSteeringReadback =
        std::chrono::duration<float>(readbackEnd - readbackStart).count();

    auto endTime = std::chrono::high_resolution_clock::now();
    g_app.lastGPUComputeTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

// ============================================================================
// Camera Control Overlay (PHASE 11 - Agent 10)
// ============================================================================
void RenderCameraControlOverlay() {
    ImGuiIO& io = ImGui::GetIO();

    // Position in bottom-right corner
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 220, io.DisplaySize.y - 140), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                              ImGuiWindowFlags_NoInputs |
                              ImGuiWindowFlags_AlwaysAutoResize |
                              ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoFocusOnAppearing |
                              ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##CameraMode", nullptr, flags)) {
        // Camera mode indicator
        const char* modeText = "";
        ImVec4 modeColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        if (g_app.mouseCaptured) {
            modeText = "FPS MODE";
            modeColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
        } else if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) {
            modeText = "FOLLOW MODE";
            modeColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
        } else {
            modeText = "ORBIT MODE";
            modeColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
        }

        ImGui::TextColored(modeColor, "%s", modeText);

        // Camera settings
        ImGui::Text("Speed: %.0f", g_app.cameraMoveSpeed);
        ImGui::Text("Sens: %.2f", g_app.mouseSensitivity);

        // Quick tips based on mode
        if (g_app.mouseCaptured) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Left Click: Release");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Left Click: Capture");
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "R: Reset Camera");
    }
    ImGui::End();
}

// ============================================================================
// Minimal FPS Overlay (Always visible in corner - Agent 30)
// ============================================================================
void RenderMinimalFPSOverlay() {
    ImGuiIO& io = ImGui::GetIO();

    // Position in bottom-left corner
    ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 80), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                              ImGuiWindowFlags_NoInputs |
                              ImGuiWindowFlags_AlwaysAutoResize |
                              ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoFocusOnAppearing |
                              ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##MinimalFPS", nullptr, flags)) {
        // FPS with color coding
        ImVec4 fpsColor = (g_app.fps >= 55.0f) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) :
                          (g_app.fps >= 30.0f) ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) :
                                                  ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        ImGui::TextColored(fpsColor, "%.0f FPS", g_app.fps);

        // Creature count
        u32 aliveCount = g_app.world.GetAliveCount();
        ImGui::Text("%u creatures", aliveCount);

        // Frame time
        ImGui::Text("%.2f ms", g_app.timings.total * 1000.0f);
    }
    ImGui::End();
}

// ============================================================================
// Stress Test Controls (Agent 30 - Performance Testing)
// ============================================================================
struct StressTestState {
    bool running = false;
    int targetCreatures = 1000;
    int step = 0;
    float timeAtStep = 0.0f;
    float measureDuration = 5.0f;  // seconds to measure at each step

    // Results
    struct Measurement {
        int creatureCount;
        float avgFPS;
        float minFPS;
        float maxFPS;
        float avgFrameTimeMs;
    };
    std::vector<Measurement> results;

    // Benchmarking targets
    std::vector<int> benchmarkTargets = {100, 250, 500, 1000, 2000, 3000, 5000, 7500, 10000};

    void reset() {
        running = false;
        step = 0;
        timeAtStep = 0.0f;
        results.clear();
    }
};
static StressTestState g_stressTest;

void UpdateStressTest(float deltaTime) {
    if (!g_stressTest.running) return;

    g_stressTest.timeAtStep += deltaTime;

    // Wait for measurement duration
    if (g_stressTest.timeAtStep >= g_stressTest.measureDuration) {
        // Record measurement
        StressTestState::Measurement m;
        m.creatureCount = g_app.world.GetAliveCount();
        m.avgFPS = g_app.fps;
        m.minFPS = g_app.fps;  // Would need history tracking for real min
        m.maxFPS = g_app.fps;
        m.avgFrameTimeMs = g_app.timings.total * 1000.0f;
        g_stressTest.results.push_back(m);

        // Move to next step
        g_stressTest.step++;
        g_stressTest.timeAtStep = 0.0f;

        if (g_stressTest.step >= static_cast<int>(g_stressTest.benchmarkTargets.size())) {
            g_stressTest.running = false;
            g_app.notifications.show("Stress test complete!", NotificationType::Success);
        } else {
            // Spawn creatures to reach next target
            int target = g_stressTest.benchmarkTargets[g_stressTest.step];
            int current = g_app.world.GetAliveCount();
            int toSpawn = target - current;

            if (toSpawn > 0) {
                for (int i = 0; i < toSpawn; ++i) {
                    float x = ((float)(std::rand() % 10000) / 10000.0f - 0.5f) * 200.0f;
                    float z = ((float)(std::rand() % 10000) / 10000.0f - 0.5f) * 200.0f;
                    CreatureType type = (std::rand() % 4 == 0) ? CreatureType::CARNIVORE : CreatureType::HERBIVORE;
                    g_app.world.SpawnCreature(glm::vec3(x, 0.0f, z), type);
                }
            }
        }
    }
}

void RenderStressTestPanel() {
    if (!ImGui::CollapsingHeader("Stress Test (Agent 30)")) return;

    if (!g_stressTest.running) {
        if (ImGui::Button("Run Benchmark")) {
            g_stressTest.reset();
            g_stressTest.running = true;
            g_app.notifications.show("Starting stress test...", NotificationType::Info);

            // Clear existing creatures for clean test
            g_app.world.Reset(100, 25, 200);  // Reset with default populations
        }

        ImGui::SliderFloat("Measure Duration (s)", &g_stressTest.measureDuration, 1.0f, 30.0f);

        ImGui::Text("Test targets:");
        for (int target : g_stressTest.benchmarkTargets) {
            ImGui::SameLine();
            ImGui::Text("%d", target);
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "STRESS TEST RUNNING...");
        ImGui::Text("Step %d / %zu", g_stressTest.step + 1, g_stressTest.benchmarkTargets.size());
        int target = g_stressTest.benchmarkTargets[g_stressTest.step];
        ImGui::Text("Target: %d creatures", target);
        ImGui::Text("Current: %u creatures", g_app.world.GetAliveCount());
        ImGui::ProgressBar(g_stressTest.timeAtStep / g_stressTest.measureDuration);

        if (ImGui::Button("Cancel")) {
            g_stressTest.running = false;
        }
    }

    // Show results
    if (!g_stressTest.results.empty()) {
        ImGui::Separator();
        ImGui::Text("Results:");
        ImGui::Columns(4, "stress_results");
        ImGui::Text("Creatures"); ImGui::NextColumn();
        ImGui::Text("FPS"); ImGui::NextColumn();
        ImGui::Text("Frame (ms)"); ImGui::NextColumn();
        ImGui::Text("Status"); ImGui::NextColumn();
        ImGui::Separator();

        for (const auto& m : g_stressTest.results) {
            ImGui::Text("%d", m.creatureCount); ImGui::NextColumn();

            ImVec4 col = (m.avgFPS >= 60.0f) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) :
                         (m.avgFPS >= 30.0f) ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) :
                                                ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            ImGui::TextColored(col, "%.1f", m.avgFPS); ImGui::NextColumn();
            ImGui::Text("%.2f", m.avgFrameTimeMs); ImGui::NextColumn();

            const char* status = (m.avgFPS >= 60.0f) ? "PASS" :
                                  (m.avgFPS >= 30.0f) ? "OK" : "FAIL";
            ImGui::Text("%s", status); ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }

    ImGui::Separator();
}

// ============================================================================
// Render Performance Overlay (Phase 2 - Detailed Frame Timing)
// ============================================================================
void RenderPerformanceOverlay() {
    if (!g_app.showPerformanceOverlay) return;

    ImGuiIO& io = ImGui::GetIO();

    // Position in top-right corner
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 360, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.85f);

    if (ImGui::Begin("Performance Profiler", &g_app.showPerformanceOverlay)) {
        const FrameTimings& t = g_app.timings;

        // Main timing display
        ImVec4 fpsColor = (g_app.fps >= 55.0f) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) :
                          (g_app.fps >= 30.0f) ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) :
                                                  ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

        ImGui::TextColored(fpsColor, "%.0f FPS", g_app.fps);
        ImGui::SameLine();
        ImGui::Text("(%.2f ms)", t.total * 1000.0f);

        ImGui::Separator();

        // Frame time breakdown
        ImGui::Text("Frame Breakdown:");

        float totalMs = t.total * 1000.0f;
        float creatureMs = t.creatureUpdate * 1000.0f;
        float gpuDispatchMs = t.gpuSteeringDispatch * 1000.0f;
        float gpuReadbackMs = t.gpuSteeringReadback * 1000.0f;
        float gpuTotalMs = gpuDispatchMs + gpuReadbackMs;
        float renderMs = t.rendering * 1000.0f;
        float postProcMs = t.postProcessing * 1000.0f;
        float uiMs = t.uiRendering * 1000.0f;
        float replayMs = t.replayRecording * 1000.0f;

        // Progress bars for each component
        ImGui::Text("  Creatures:");
        ImGui::SameLine(120);
        ImGui::ProgressBar(creatureMs / std::max(totalMs, 0.1f), ImVec2(120, 0), "");
        ImGui::SameLine();
        ImGui::Text("%.2f ms", creatureMs);

        ImGui::Text("  GPU Compute:");
        ImGui::SameLine(120);
        ImGui::ProgressBar(gpuTotalMs / std::max(totalMs, 0.1f), ImVec2(120, 0), "");
        ImGui::SameLine();
        ImGui::Text("%.2f ms", gpuTotalMs);

        ImGui::Text("    - Dispatch:");
        ImGui::SameLine(120);
        ImGui::Text("%.2f ms", gpuDispatchMs);

        ImGui::Text("    - Readback:");
        ImGui::SameLine(120);
        ImGui::Text("%.2f ms", gpuReadbackMs);

        ImGui::Text("  Rendering:");
        ImGui::SameLine(120);
        ImGui::ProgressBar(renderMs / std::max(totalMs, 0.1f), ImVec2(120, 0), "");
        ImGui::SameLine();
        ImGui::Text("%.2f ms", renderMs);

        ImGui::Text("  Post-Process:");
        ImGui::SameLine(120);
        ImGui::ProgressBar(postProcMs / std::max(totalMs, 0.1f), ImVec2(120, 0), "");
        ImGui::SameLine();
        ImGui::Text("%.2f ms", postProcMs);

        ImGui::Text("  UI:");
        ImGui::SameLine(120);
        ImGui::ProgressBar(uiMs / std::max(totalMs, 0.1f), ImVec2(120, 0), "");
        ImGui::SameLine();
        ImGui::Text("%.2f ms", uiMs);

        ImGui::Text("  Replay:");
        ImGui::SameLine(120);
        ImGui::ProgressBar(replayMs / std::max(totalMs, 0.1f), ImVec2(120, 0), "");
        ImGui::SameLine();
        ImGui::Text("%.2f ms", replayMs);

        ImGui::Separator();

        // Frame time graph
        ImGui::Text("Frame Time History (120 frames):");
        ImGui::PlotLines("##frametime", t.frameTimeHistory, FrameTimings::HISTORY_SIZE,
                         static_cast<int>(t.historyIndex), nullptr, 0.0f, 33.3f, ImVec2(0, 60));

        // FPS graph
        ImGui::Text("FPS History:");
        ImGui::PlotLines("##fps", t.fpsHistory, FrameTimings::HISTORY_SIZE,
                         static_cast<int>(t.historyIndex), nullptr, 0.0f, 120.0f, ImVec2(0, 60));

        ImGui::Separator();

        // Performance targets
        ImGui::Text("Performance Targets:");
        u32 aliveCount = g_app.world.GetAliveCount();

        auto targetIndicator = [](bool met, const char* label) {
            if (met) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK]");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[!!]");
            }
            ImGui::SameLine();
            ImGui::Text("%s", label);
        };

        targetIndicator(aliveCount < 1000 || g_app.fps >= 60.0f, "1K creatures @ 60 FPS");
        targetIndicator(aliveCount < 5000 || g_app.fps >= 30.0f, "5K creatures @ 30 FPS");
        targetIndicator(aliveCount < 10000 || g_app.fps >= 15.0f, "10K creatures @ 15 FPS");

        ImGui::Separator();

        // Memory estimate
        size_t creatureMemory;
        if (g_app.world.usePooling) {
            creatureMemory = g_app.world.creaturePool.getTotalCapacity() * sizeof(SimCreature);
        } else {
            creatureMemory = g_app.world.creatures.size() * sizeof(SimCreature);
        }
        size_t foodMemory = g_app.world.foods.size() * sizeof(Food);
        ImGui::Text("Memory (approx):");
        ImGui::Text("  Creatures: %.2f KB", creatureMemory / 1024.0f);
        ImGui::Text("  Food: %.2f KB", foodMemory / 1024.0f);
        if (g_app.world.usePooling) {
            ImGui::Text("  (Pool mode - preallocated)");
        }

        // Memory Leak Detection (Agent 30)
        ImGui::Separator();
        ImGui::Text("Memory Leak Detection:");
        static size_t lastCreatureMemory = 0;
        static size_t peakCreatureMemory = 0;
        static int stableFrameCount = 0;
        static float memoryGrowthRate = 0.0f;

        if (creatureMemory > peakCreatureMemory) {
            peakCreatureMemory = creatureMemory;
        }

        // Calculate growth rate
        if (lastCreatureMemory > 0 && creatureMemory > lastCreatureMemory) {
            memoryGrowthRate = (creatureMemory - lastCreatureMemory) / 1024.0f;  // KB/frame
            stableFrameCount = 0;
        } else {
            stableFrameCount++;
            memoryGrowthRate *= 0.95f;  // Decay
        }
        lastCreatureMemory = creatureMemory;

        ImGui::Text("  Peak: %.2f KB", peakCreatureMemory / 1024.0f);
        ImGui::Text("  Growth Rate: %.3f KB/frame", memoryGrowthRate);

        if (stableFrameCount > 60 * 5) {  // 5 seconds stable
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "  Status: STABLE");
        } else if (memoryGrowthRate > 1.0f) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "  Status: GROWING");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "  Status: MONITORING");
        }

        // Pool health check
        if (g_app.world.usePooling) {
            const auto& pool = g_app.world.creaturePool;
            u32 active = pool.getActiveCount();
            u32 capacity = pool.getTotalCapacity();
            u32 releases = pool.getTotalReleases();
            u32 acquisitions = pool.getTotalAcquisitions();

            // Check for leak: acquisitions - releases should roughly match active
            int expectedActive = static_cast<int>(acquisitions) - static_cast<int>(releases);
            int actualActive = static_cast<int>(active);
            int diff = std::abs(expectedActive - actualActive);

            ImGui::Separator();
            ImGui::Text("Pool Health:");
            ImGui::Text("  Expected Active: %d", expectedActive);
            ImGui::Text("  Actual Active: %d", actualActive);
            if (diff > 10) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "  Mismatch: %d (LEAK?)", diff);
            } else {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "  Mismatch: %d (OK)", diff);
            }
        }
    }
    ImGui::End();
}

// ============================================================================
// Render Debug Panel
// ============================================================================
void RenderDebugPanel() {
    if (!g_app.showDebugPanel) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Simulation Debug", &g_app.showDebugPanel)) {
        // Performance section
        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", g_app.fps);
            ImGui::Text("Frame Time: %.2f ms", g_app.frameTimeMs);
            ImGui::Text("Delta Time: %.4f s", g_app.deltaTime);
            ImGui::Checkbox("Show Profiler (F2)", &g_app.showPerformanceOverlay);

            // Performance targets (Agent 30)
            ImGui::Separator();
            ImGui::Text("Performance Targets:");
            u32 aliveCount = g_app.world.GetAliveCount();
            auto targetIndicator = [](bool met, const char* label) {
                if (met) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK]");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[!!]");
                }
                ImGui::SameLine();
                ImGui::Text("%s", label);
            };
            targetIndicator(aliveCount < 1000 || g_app.fps >= 60.0f, "60 FPS @ 1K");
            ImGui::SameLine();
            targetIndicator(aliveCount < 2000 || g_app.fps >= 60.0f, "60 FPS @ 2K");
            ImGui::SameLine();
            targetIndicator(aliveCount < 5000 || g_app.fps >= 30.0f, "30 FPS @ 5K");

            // Memory Usage (Agent 30)
            ImGui::Separator();
            ImGui::Text("Memory:");
            size_t creatureMemory;
            if (g_app.world.usePooling) {
                creatureMemory = g_app.world.creaturePool.getTotalCapacity() * sizeof(SimCreature);
            } else {
                creatureMemory = g_app.world.creatures.size() * sizeof(SimCreature);
            }
            size_t foodMemory = g_app.world.foods.size() * sizeof(Food);
            ImGui::Text("  Creatures: %.2f MB", creatureMemory / (1024.0f * 1024.0f));
            ImGui::Text("  Food: %.2f KB", foodMemory / 1024.0f);

            ImGui::Separator();
        }

        // Stress Test section (Agent 30)
        RenderStressTestPanel();

        // GPU Steering section - Agent 36: LOUD status notifications
        if (ImGui::CollapsingHeader("GPU Steering", ImGuiTreeNodeFlags_DefaultOpen)) {
            // LOUD status indicator - users MUST know what they're getting
            if (g_app.gpuSteeringAvailable && g_app.gpuSteeringEnabled) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                ImGui::Text("*** GPU STEERING: ENABLED ***");
                ImGui::PopStyleColor();
                ImGui::Text("Compute shader active");
            } else if (g_app.gpuSteeringAvailable && !g_app.gpuSteeringEnabled) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
                ImGui::Text("*** GPU AVAILABLE BUT DISABLED ***");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::Text("*** GPU NOT AVAILABLE - CPU FALLBACK ***");
                ImGui::PopStyleColor();
                ImGui::Text("Performance limited for >200 creatures");
            }
            ImGui::Separator();
            ImGui::Text("Available: %s", g_app.gpuSteeringAvailable ? "Yes" : "No");
            if (g_app.gpuSteeringAvailable) {
                ImGui::Checkbox("Enabled", &g_app.gpuSteeringEnabled);
                ImGui::Text("Threshold: %u creatures", GPU_STEERING_THRESHOLD);
                ImGui::Text("Dispatch Count: %u", g_app.gpuSteeringDispatchCount);
                ImGui::Text("Last Compute Time: %.2f ms", g_app.lastGPUComputeTime);
                if (g_app.gpuSteering && g_app.gpuSteering->IsInitialized()) {
                    ImGui::Text("Last Dispatch: %u creatures", g_app.gpuSteering->GetLastDispatchCount());
                }
            }
            ImGui::Separator();
        }

        // Simulation section
        if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Time: %.2f s", g_app.world.simulationTime);

            ImGui::Checkbox("Paused", &g_app.world.paused);
            ImGui::SliderFloat("Time Scale", &g_app.world.timeScale, 0.1f, 10.0f);

            ImGui::Separator();
            ImGui::Text("Population:");
            ImGui::Text("  Total Alive: %u", g_app.world.GetAliveCount());
            ImGui::Text("  Herbivores: %u", g_app.world.herbivoreCount);
            ImGui::Text("  Carnivores: %u", g_app.world.carnivoreCount);
            ImGui::Text("  Food Sources: %zu", g_app.world.foods.size());

            ImGui::Separator();
            ImGui::Text("Overlays:");
            ImGui::Checkbox("Show Nametags", &g_app.showNametags);
            ImGui::SliderFloat("Nametag Distance", &g_app.nametagMaxDistance, 10.0f, 500.0f, "%.0f");

            ImGui::Separator();
            ImGui::Text("Spawn Controls:");

            static int spawnHerbivores = 50;
            static int spawnCarnivores = 10;
            static int spawnFlying = 15;
            static int spawnAquatic = 25;
            static int spawnFood = 200;
            static float spawnRadius = 0.0f;
            static float spawnEnergyMin = 80.0f;
            static float spawnEnergyMax = 120.0f;
            static float spawnFoodMin = 30.0f;
            static float spawnFoodMax = 60.0f;

            ImGui::InputInt("Herbivores##Spawn", &spawnHerbivores);
            ImGui::InputInt("Carnivores##Spawn", &spawnCarnivores);
            ImGui::InputInt("Flying##Spawn", &spawnFlying);
            ImGui::InputInt("Aquatic##Spawn", &spawnAquatic);
            ImGui::InputInt("Food##Spawn", &spawnFood);
            float spawnRadiusMax = std::max(50.0f, g_app.world.GetWorldBounds());
            if (spawnRadius <= 0.0f) {
                spawnRadius = std::min(spawnRadiusMax, g_app.world.GetWorldBounds() * 0.6f);
            }
            ImGui::SliderFloat("Spawn Radius", &spawnRadius, 10.0f, spawnRadiusMax);
            ImGui::SliderFloat("Energy Min", &spawnEnergyMin, 10.0f, 200.0f);
            ImGui::SliderFloat("Energy Max", &spawnEnergyMax, 10.0f, 200.0f);
            ImGui::SliderFloat("Food Min", &spawnFoodMin, 5.0f, 100.0f);
            ImGui::SliderFloat("Food Max", &spawnFoodMax, 5.0f, 100.0f);

            if (ImGui::Button("Spawn Herbivores")) {
                g_app.world.SpawnCreatures(
                    CreatureType::HERBIVORE,
                    static_cast<u32>(std::max(0, spawnHerbivores)),
                    spawnRadius,
                    spawnEnergyMin,
                    spawnEnergyMax
                );
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Carnivores")) {
                g_app.world.SpawnCreatures(
                    CreatureType::CARNIVORE,
                    static_cast<u32>(std::max(0, spawnCarnivores)),
                    spawnRadius,
                    spawnEnergyMin,
                    spawnEnergyMax
                );
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Flying")) {
                g_app.world.SpawnCreatures(
                    CreatureType::FLYING,
                    static_cast<u32>(std::max(0, spawnFlying)),
                    spawnRadius,
                    spawnEnergyMin,
                    spawnEnergyMax
                );
            }
            if (ImGui::Button("Spawn Aquatic")) {
                g_app.world.SpawnCreatures(
                    CreatureType::AQUATIC,
                    static_cast<u32>(std::max(0, spawnAquatic)),
                    spawnRadius,
                    spawnEnergyMin,
                    spawnEnergyMax
                );
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Food")) {
                g_app.world.SpawnFood(
                    static_cast<u32>(std::max(0, spawnFood)),
                    spawnRadius,
                    spawnFoodMin,
                    spawnFoodMax
                );
            }

            static int resetHerbivores = 1000;
            static int resetCarnivores = 100;
            static int resetFood = 500;
            ImGui::Separator();
            ImGui::Text("Reset Population:");
            ImGui::InputInt("Herbivores##Reset", &resetHerbivores);
            ImGui::InputInt("Carnivores##Reset", &resetCarnivores);
            ImGui::InputInt("Food##Reset", &resetFood);
            if (ImGui::Button("Reset Population")) {
                g_app.world.Reset(
                    static_cast<u32>(std::max(0, resetHerbivores)),
                    static_cast<u32>(std::max(0, resetCarnivores)),
                    static_cast<u32>(std::max(0, resetFood))
                );
            }

            ImGui::Separator();
            ImGui::Text("Statistics:");
            ImGui::Text("  Total Births: %u", g_app.world.totalBirths);
            ImGui::Text("  Total Deaths: %u", g_app.world.totalDeaths);
            ImGui::Separator();
        }

        // Creature Pool section
        if (ImGui::CollapsingHeader("Creature Pool", ImGuiTreeNodeFlags_DefaultOpen)) {
            const CreaturePool& pool = g_app.world.creaturePool;

            ImGui::Checkbox("Use Pooling", &g_app.world.usePooling);

            if (g_app.world.usePooling) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "POOL ACTIVE");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "LEGACY MODE");
            }

            ImGui::Separator();
            ImGui::Text("Pool Status:");
            ImGui::Text("  Active: %u", pool.getActiveCount());
            ImGui::Text("  Available: %u", pool.getAvailableCount());
            ImGui::Text("  Total Capacity: %u", pool.getTotalCapacity());

            // Utilization bar
            float utilization = pool.getUtilization();
            ImVec4 utilColor = (utilization < 0.7f) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) :
                               (utilization < 0.9f) ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) :
                                                       ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            ImGui::Text("  Utilization:");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, utilColor);
            ImGui::ProgressBar(utilization, ImVec2(100, 0), "");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%.1f%%", utilization * 100.0f);

            ImGui::Separator();
            ImGui::Text("Statistics:");
            ImGui::Text("  Peak Active: %u", pool.getPeakActiveCount());
            ImGui::Text("  Total Acquisitions: %u", pool.getTotalAcquisitions());
            ImGui::Text("  Total Releases: %u", pool.getTotalReleases());
            ImGui::Text("  Pool Growths: %u", pool.getPoolGrowthCount());

            // Memory savings estimate
            size_t poolMemory = pool.getTotalCapacity() * sizeof(SimCreature);
            size_t wouldAllocate = pool.getTotalAcquisitions() * sizeof(SimCreature);
            float savings = (wouldAllocate > 0) ?
                (1.0f - (float)poolMemory / (float)wouldAllocate) * 100.0f : 0.0f;
            ImGui::Text("  Memory Saved: ~%.1f%%", std::max(0.0f, savings));
            ImGui::Separator();
        }

        // Camera section
        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::SliderFloat("Yaw", &g_app.cameraYaw, -180.0f, 180.0f);
            ImGui::SliderFloat("Pitch", &g_app.cameraPitch, 10.0f, 89.0f);
            ImGui::SliderFloat("Distance", &g_app.cameraDistance, 50.0f, 500.0f);

            ImGui::Text("Position: (%.1f, %.1f, %.1f)",
                g_app.cameraPosition.x, g_app.cameraPosition.y, g_app.cameraPosition.z);
        }

        // Performance Settings section (Agent 30)
        if (ImGui::CollapsingHeader("Performance Settings")) {
            ImGui::Text("Quality Presets:");
            static int qualityPreset = 2;  // Medium by default
            const char* presets[] = {"Ultra Low", "Low", "Medium", "High", "Ultra"};
            if (ImGui::Combo("Preset", &qualityPreset, presets, 5)) {
                // Apply preset settings
                switch (qualityPreset) {
                    case 0:  // Ultra Low
                        g_app.waterRenderingEnabled = false;
                        g_app.grassRenderingEnabled = false;
                        g_app.treeRenderingEnabled = false;
                        g_app.terrainRenderingEnabled = false;
                        g_app.showNametags = false;
                        break;
                    case 1:  // Low
                        g_app.waterRenderingEnabled = false;
                        g_app.grassRenderingEnabled = false;
                        g_app.treeRenderingEnabled = true;
                        g_app.terrainRenderingEnabled = true;
                        g_app.showNametags = true;
                        g_app.nametagMaxDistance = 100.0f;
                        break;
                    case 2:  // Medium
                        g_app.waterRenderingEnabled = true;
                        g_app.grassRenderingEnabled = false;
                        g_app.treeRenderingEnabled = true;
                        g_app.terrainRenderingEnabled = true;
                        g_app.showNametags = true;
                        g_app.nametagMaxDistance = 200.0f;
                        break;
                    case 3:  // High
                        g_app.waterRenderingEnabled = true;
                        g_app.grassRenderingEnabled = true;
                        g_app.treeRenderingEnabled = true;
                        g_app.terrainRenderingEnabled = true;
                        g_app.showNametags = true;
                        g_app.nametagMaxDistance = 300.0f;
                        break;
                    case 4:  // Ultra
                        g_app.waterRenderingEnabled = true;
                        g_app.grassRenderingEnabled = true;
                        g_app.treeRenderingEnabled = true;
                        g_app.terrainRenderingEnabled = true;
                        g_app.showNametags = true;
                        g_app.nametagMaxDistance = 500.0f;
                        break;
                }
            }

            ImGui::Separator();
            ImGui::Text("Individual Settings:");

            // Creature limits
            static int maxCreaturesLimit = 10000;
            if (ImGui::SliderInt("Max Creatures", &maxCreaturesLimit, 500, 65536)) {
                // Note: This is a soft limit displayed to user; actual pool growth is handled by CreaturePool
            }
            ImGui::Text("  Current: %u / %d", g_app.world.GetAliveCount(), maxCreaturesLimit);

            // Rendering toggles
            ImGui::Checkbox("Water Effects", &g_app.waterRenderingEnabled);
            ImGui::Checkbox("Grass Rendering", &g_app.grassRenderingEnabled);
            ImGui::Checkbox("Tree Rendering", &g_app.treeRenderingEnabled);
            ImGui::Checkbox("Terrain Rendering", &g_app.terrainRenderingEnabled);

            // Nametag settings
            ImGui::Separator();
            ImGui::Checkbox("Show Nametags", &g_app.showNametags);
            if (g_app.showNametags) {
                ImGui::SliderFloat("Nametag Distance", &g_app.nametagMaxDistance, 10.0f, 500.0f);
            }

            // GPU Compute toggle
            ImGui::Separator();
            if (g_app.gpuSteeringAvailable) {
                ImGui::Checkbox("GPU Steering Compute", &g_app.gpuSteeringEnabled);
                if (g_app.gpuSteeringEnabled) {
                    ImGui::Text("  Threshold: %u creatures", GPU_STEERING_THRESHOLD);
                }
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "GPU Steering: Not Available");
            }

            ImGui::Separator();
        }

        // Water Rendering section (Phase 5 - 3D World)
        if (ImGui::CollapsingHeader("Water Rendering")) {
            ImGui::Checkbox("Enabled", &g_app.waterRenderingEnabled);

            if (g_app.waterRenderer.IsInitialized()) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "INITIALIZED");
                ImGui::Text("Vertices: %d", g_app.waterRenderer.GetVertexCount());
                ImGui::Text("Indices: %d", g_app.waterRenderer.GetIndexCount());

                ImGui::Separator();
                ImGui::Text("Water Height: %.1f", g_app.waterLevel);

                // Wave parameters (stored in water renderer, just showing info)
                ImGui::Text("Wave animation active");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "NOT INITIALIZED");
            }
        }

        // Grass Rendering section (Phase 5 - 3D World)
        if (ImGui::CollapsingHeader("Grass Rendering")) {
            ImGui::Checkbox("Enabled", &g_app.grassRenderingEnabled);

            if (g_app.grassRenderer) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "INITIALIZED");
                ImGui::Text("Visible Blades: %d", g_app.grassRenderer->getVisibleBladeCount());

                ImGui::Separator();
                ImGui::Text("Wind animation active");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "NOT INITIALIZED");
            }
        }

        // Tree Rendering section (Phase 5 - 3D World)
        if (ImGui::CollapsingHeader("Tree Rendering")) {
            ImGui::Checkbox("Enabled##Trees", &g_app.treeRenderingEnabled);

            if (g_app.treeRenderer) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "INITIALIZED");
                ImGui::Text("Total Trees: %d", g_app.treeRenderer->getTotalTreeCount());
                ImGui::Text("Rendered: %d", g_app.treeRenderer->getRenderedTreeCount());
                ImGui::Text("Culled: %d", g_app.treeRenderer->getCulledTreeCount());
                ImGui::Text("Draw Calls: %d", g_app.treeRenderer->getDrawCallCount());

                ImGui::Separator();
                glm::vec2 windDir = g_app.treeRenderer->getWindDirection();
                float windStrength = g_app.treeRenderer->getWindStrength();
                ImGui::Text("Wind: (%.2f, %.2f) x %.2f", windDir.x, windDir.y, windStrength);

                if (ImGui::SliderFloat("Wind Strength", &windStrength, 0.0f, 1.0f)) {
                    g_app.treeRenderer->setWindStrength(windStrength);
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "NOT INITIALIZED");
            }
        }

        // Replay Controls (S-05, S-06)
        if (ImGui::CollapsingHeader("Replay System", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Recording indicator
            if (g_app.isRecording && !g_app.isPlayingReplay) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "● REC");
                ImGui::SameLine();
                ImGui::Text("%.1f min (%zu frames)",
                    g_app.replayRecorder.getDuration() / 60.0f,
                    g_app.replayRecorder.getFrameCount());
            }

            if (!g_app.isPlayingReplay) {
                if (ImGui::Button("Enter Replay Mode (F10)")) {
                    EnterReplayMode();
                }

                ImGui::Separator();

                // Save replay
                static char filename[256] = "replay.rpl";
                ImGui::InputText("Filename", filename, sizeof(filename));
                if (ImGui::Button("Save Replay")) {
                    std::string path = std::string("replays/") + filename;
                    CreateDirectoryA("replays", nullptr);
                    if (g_app.replayRecorder.saveReplay(path)) {
                        g_app.statusMessage = "Saved: " + path;
                        g_app.statusMessageTimer = 3.0f;
                    } else {
                        g_app.statusMessage = "Failed to save replay";
                        g_app.statusMessageTimer = 3.0f;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Load Replay")) {
                    std::string path = std::string("replays/") + filename;
                    if (g_app.replayPlayer.loadReplay(path)) {
                        EnterReplayMode();
                    } else {
                        g_app.statusMessage = "Failed to load: " + path;
                        g_app.statusMessageTimer = 3.0f;
                    }
                }
            } else {
                // Playback mode UI
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "▶ PLAYBACK MODE");

                // Timeline scrubber
                float progress = g_app.replayPlayer.getProgress();
                if (ImGui::SliderFloat("Timeline", &progress, 0.0f, 1.0f, "%.1f%%")) {
                    g_app.replayPlayer.seekPercent(progress);
                }

                ImGui::Text("Frame: %zu / %zu",
                    g_app.replayPlayer.getCurrentFrameIndex() + 1,
                    g_app.replayPlayer.getTotalFrames());
                ImGui::Text("Time: %.1f / %.1f sec",
                    g_app.replayPlayer.getCurrentTime(),
                    g_app.replayPlayer.getDuration());

                // Playback controls
                if (ImGui::Button(g_app.replayPlayer.isPaused() ? "Play" : "Pause")) {
                    g_app.replayPlayer.togglePause();
                }
                ImGui::SameLine();
                if (ImGui::Button("<<")) g_app.replayPlayer.stepBackward();
                ImGui::SameLine();
                if (ImGui::Button(">>")) g_app.replayPlayer.stepForward();

                // Speed control
                float speed = g_app.replayPlayer.getSpeed();
                if (ImGui::SliderFloat("Speed", &speed, 0.25f, 4.0f, "%.2fx")) {
                    g_app.replayPlayer.setSpeed(speed);
                }

                ImGui::Separator();
                if (ImGui::Button("Exit Replay (F10)")) {
                    ExitReplayMode();
                }
            }
            ImGui::Separator();
        }

        // Save/Load section
        if (ImGui::CollapsingHeader("Save/Load")) {
            static char saveFilename[256] = "mysave.evos";
            ImGui::InputText("Filename", saveFilename, sizeof(saveFilename));

            if (ImGui::Button("Save Game")) {
                SaveGame(saveFilename);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Game")) {
                LoadGame(saveFilename);
            }

            ImGui::Separator();

            if (ImGui::Button("Quick Save (F5)")) {
                QuickSave();
            }
            ImGui::SameLine();
            if (ImGui::Button("Quick Load (F9)")) {
                QuickLoad();
            }

            ImGui::Separator();

            // Auto-save settings
            static bool autoSaveEnabled = true;
            if (ImGui::Checkbox("Auto-save", &autoSaveEnabled)) {
                if (autoSaveEnabled) {
                    g_app.saveManager.enableAutoSave(300.0f);  // 5 minutes
                } else {
                    g_app.saveManager.disableAutoSave();
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(every 5 min)");

            ImGui::Separator();
            ImGui::Text("Save directory: %s", g_app.saveManager.getSaveDirectory().c_str());

            // List existing saves with load/delete buttons
            auto saves = g_app.saveManager.listSaveSlots();
            if (!saves.empty()) {
                ImGui::Text("Saved games (%zu):", saves.size());
                static int selectedSaveIdx = -1;
                static std::string deleteConfirmFile;

                for (size_t i = 0; i < saves.size(); ++i) {
                    const auto& save = saves[i];
                    ImGui::PushID(static_cast<int>(i));

                    // Selectable row
                    bool isSelected = (selectedSaveIdx == static_cast<int>(i));
                    if (ImGui::Selectable("##select", isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                        selectedSaveIdx = static_cast<int>(i);
#ifdef _WIN32
                        strncpy_s(saveFilename, sizeof(saveFilename), save.filename.c_str(), _TRUNCATE);
#else
                        std::strncpy(saveFilename, save.filename.c_str(), sizeof(saveFilename) - 1);
                        saveFilename[sizeof(saveFilename) - 1] = '\0';
#endif
                    }
                    ImGui::SameLine();

                    // Format timestamp
                    char timeStr[64] = "Unknown";
                    if (save.timestamp > 0) {
                        std::time_t t = static_cast<std::time_t>(save.timestamp);
#ifdef _WIN32
                        std::tm tm{};
                        if (localtime_s(&tm, &t) == 0) {
                            std::strftime(timeStr, sizeof(timeStr), "%m/%d %H:%M", &tm);
                        }
#else
                        std::tm* tm = std::localtime(&t);
                        if (tm) {
                            std::strftime(timeStr, sizeof(timeStr), "%m/%d %H:%M", tm);
                        }
#endif
                    }

                    // Save info with buttons
                    ImGui::Text("%s", save.displayName.c_str());
                    ImGui::SameLine(200);
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%u creatures", save.creatureCount);
                    ImGui::SameLine(320);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", timeStr);
                    ImGui::SameLine(400);

                    // Load button
                    if (ImGui::SmallButton("Load")) {
                        LoadGame(save.filename);
                    }
                    ImGui::SameLine();

                    // Delete button (with confirmation)
                    if (deleteConfirmFile == save.filename) {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Delete?");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Yes")) {
                            g_app.saveManager.deleteSave(save.filename);
                            deleteConfirmFile.clear();
                            g_app.statusMessage = "Deleted: " + save.displayName;
                            g_app.statusMessageTimer = 2.0f;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("No")) {
                            deleteConfirmFile.clear();
                        }
                    } else {
                        if (ImGui::SmallButton("Del")) {
                            deleteConfirmFile = save.filename;
                        }
                    }

                    ImGui::PopID();
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No saved games yet");
            }
        }

        // Controls help
        if (ImGui::CollapsingHeader("Controls")) {
            ImGui::BulletText("WASD: Move camera");
            ImGui::BulletText("Mouse Drag: Rotate camera");
            ImGui::BulletText("Scroll: Zoom");
            ImGui::BulletText("P: Toggle pause");
            ImGui::BulletText("F1: Toggle debug panel");
            ImGui::BulletText("F2: Toggle performance profiler");
            ImGui::BulletText("F5: Quick save");
            ImGui::BulletText("F9: Quick load");
            ImGui::BulletText("F10: Toggle replay mode");
            ImGui::BulletText("ESC: Exit");
        }
    }
    ImGui::End();

    // Status message overlay
    if (g_app.statusMessageTimer > 0.0f) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, 50), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGui::Begin("Status", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%s", g_app.statusMessage.c_str());
        ImGui::End();
    }

    // REPLAY indicator overlay when in playback mode
    if (g_app.isPlayingReplay) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.8f, 0.1f, 0.1f, 0.8f));
        ImGui::Begin("ReplayIndicator", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("  REPLAY  ");
        ImGui::Text("Frame %zu/%zu",
            g_app.replayPlayer.getCurrentFrameIndex() + 1,
            g_app.replayPlayer.getTotalFrames());
        ImGui::Text("%.1fs / %.1fs",
            g_app.replayPlayer.getCurrentTime(),
            g_app.replayPlayer.getDuration());
        if (g_app.replayPlayer.isPaused()) {
            ImGui::Text("  PAUSED  ");
        }
        ImGui::PopStyleColor();
        ImGui::End();
        ImGui::PopStyleColor();
    }
}

// ============================================================================
// Main Frame Update (with profiling)
// ============================================================================
void UpdateFrame() {
    auto frameStart = std::chrono::high_resolution_clock::now();
    LogWorldDiag("UpdateFrame begin.");

    // Reset per-frame timings
    g_app.timings.reset();

    // Calculate delta time
    auto currentTime = std::chrono::high_resolution_clock::now();
    g_app.deltaTime = std::chrono::duration<float>(currentTime - g_app.lastFrameTime).count();
    g_app.lastFrameTime = currentTime;

    // Cap delta time to avoid spiral of death
    if (g_app.deltaTime > 0.1f) {
        g_app.deltaTime = 0.1f;
    }

    // Update FPS counter
    static float fpsAccumulator = 0.0f;
    static int fpsFrameCount = 0;
    fpsAccumulator += g_app.deltaTime;
    fpsFrameCount++;
    if (fpsAccumulator >= 1.0f) {
        g_app.fps = static_cast<float>(fpsFrameCount) / fpsAccumulator;
        g_app.frameTimeMs = (fpsAccumulator / fpsFrameCount) * 1000.0f;
        fpsAccumulator = 0.0f;
        fpsFrameCount = 0;
    }

    // Update status message timer
    if (g_app.statusMessageTimer > 0.0f) {
        g_app.statusMessageTimer -= g_app.deltaTime;
    }

    if (g_app.worldGenInProgress) {
        g_app.loadingPulseTime += g_app.deltaTime;
        g_app.worldGenElapsed += g_app.deltaTime;

        {
            std::lock_guard<std::mutex> lock(g_app.worldGenMutex);
            if (!g_app.worldGenStage.empty()) {
                g_app.loadingStatus = g_app.worldGenStage;
            }
            float progress = std::clamp(g_app.worldGenProgress, 0.0f, 0.95f);
            g_app.loadingProgress = std::max(g_app.loadingProgress, progress);
        }

        if (g_app.worldGenFuture.valid()) {
            if (g_app.worldGenFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                try {
                    g_app.worldGenFuture.get();
                    AppendRuntimeDiagLog("World generation future ready. Applying world data...");
                    ApplyGeneratedWorldData(g_app.pendingProceduralConfig,
                                            g_app.pendingEvolutionPreset,
                                            g_app.pendingGodMode);
                    AppendRuntimeDiagLog("ApplyGeneratedWorldData returned.");
                } catch (const std::exception& ex) {
                    g_app.statusMessage = std::string("World generation failed: ") + ex.what();
                    g_app.statusMessageTimer = 5.0f;
                    g_app.isLoading = false;
                    g_app.worldGenInProgress = false;
                    g_app.mainMenu.setActive(true);
                    AppendWorldGenMainLog(std::string("World generation failed: ") + ex.what());
                }
            }
        } else {
            g_app.isLoading = false;
            g_app.worldGenInProgress = false;
            AppendWorldGenMainLog("World generation future invalid.");
        }
    }

    const bool menuActive = g_app.mainMenu.isActive();
    const bool worldReady = g_app.hasGeneratedWorld && !g_app.isLoading;
    if (g_app.worldDiagnostics && g_app.worldDiagnosticsFrames > 0) {
        LogWorldDiag(std::string("UpdateFrame state: menuActive=") +
                     (menuActive ? "true" : "false") +
                     " worldReady=" + (worldReady ? "true" : "false") +
                     " loading=" + (g_app.isLoading ? "true" : "false") +
                     " unified=" + (g_app.useUnifiedSimulation ? "true" : "false"));
    }

    // Update notifications (Phase 5)
    g_app.notifications.update(g_app.deltaTime);

    // Update day/night cycle
    if (!menuActive && worldReady && !g_app.world.paused && !g_app.isPlayingReplay) {
        g_app.dayNight.Update(g_app.deltaTime * g_app.world.timeScale);
    }

    // Update auto-save timer (only when simulation is running)
    if (!menuActive && worldReady && !g_app.world.paused && !g_app.isPlayingReplay) {
        g_app.saveManager.update(g_app.deltaTime);
    }

    // Update camera transition (Phase 5)
    if (g_app.cameraTransition.active) {
        g_app.cameraTransition.update(g_app.deltaTime);

        if (g_app.cameraTransition.usePositionTarget) {
            // Position-based transition (new smooth camera)
            g_app.cameraPosition = g_app.cameraTransition.getInterpolatedPosition();
            g_app.cameraTarget = g_app.cameraTransition.getInterpolatedTarget();
        } else {
            // Original orbit-based transition
            g_app.cameraTarget = g_app.cameraTransition.getInterpolatedTarget();
            g_app.cameraYaw = g_app.cameraTransition.getInterpolatedValue(
                g_app.cameraTransition.startYaw, g_app.cameraTransition.endYaw);
            g_app.cameraPitch = g_app.cameraTransition.getInterpolatedValue(
                g_app.cameraTransition.startPitch, g_app.cameraTransition.endPitch);
            g_app.cameraDistance = g_app.cameraTransition.getInterpolatedValue(
                g_app.cameraTransition.startDistance, g_app.cameraTransition.endDistance);
        }
    }

    // Update follow camera mode
    if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE && !g_app.cameraTransition.active) {
        bool hasTarget = false;
        glm::vec3 creaturePos(0.0f);
        glm::vec3 creatureDir(0.0f, 0.0f, 1.0f);

        if (g_app.useUnifiedSimulation) {
            if (g_app.followCreature && g_app.followCreature->isAlive()) {
                creaturePos = g_app.followCreature->getPosition();
                creatureDir = g_app.followCreature->getVelocity();
                if (glm::length(creatureDir) < 0.01f) {
                    creatureDir = glm::vec3(0.0f, 0.0f, 1.0f);
                } else {
                    creatureDir = glm::normalize(creatureDir);
                }
                hasTarget = true;
            }
        } else if (g_app.followCreatureId >= 0) {
            creaturePos = GetCreaturePosition(g_app.followCreatureId);
            creatureDir = GetCreatureVelocity(g_app.followCreatureId);
            hasTarget = true;
        }

        if (!hasTarget) {
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            g_app.followCreatureId = -1;
            g_app.followCreature = nullptr;
        } else {

            // Apply user orbit angle offset
            float angle = atan2(creatureDir.z, creatureDir.x) + glm::radians(g_app.followOrbitAngle);
            glm::vec3 offsetDir = glm::vec3(cos(angle), 0.0f, sin(angle));

            // Calculate desired camera position
            glm::vec3 desiredPos = creaturePos - offsetDir * g_app.followDistance +
                                   glm::vec3(0.0f, g_app.followHeight, 0.0f);

            // Smooth interpolation to follow creature
            g_app.cameraPosition = SmoothDamp(g_app.cameraPosition, desiredPos,
                                              g_app.followVelocity,
                                              1.0f / g_app.followSmoothing, g_app.deltaTime);

            // Look at creature (slightly above ground)
            g_app.cameraTarget = creaturePos + glm::vec3(0.0f, 2.0f, 0.0f);
        }
    }

    // Update cinematic camera mode
    if (g_app.cinematicPlaying) {
        g_app.cinematicTime += g_app.deltaTime;

        // Slow pan around the world
        float radius = 150.0f;
        float height = 80.0f;
        float panSpeed = 0.1f;

        g_app.cameraPosition.x = radius * cos(g_app.cinematicTime * panSpeed);
        g_app.cameraPosition.z = radius * sin(g_app.cinematicTime * panSpeed);
        g_app.cameraPosition.y = height + 20.0f * sin(g_app.cinematicTime * panSpeed * 0.5f);

        g_app.cameraTarget = glm::vec3(0.0f, 10.0f, 0.0f);

        // End cinematic after 60 seconds
        if (g_app.cinematicTime > 60.0f) {
            g_app.cinematicPlaying = false;
            g_app.currentPreset = AppState::CameraPreset::FREE;
        }
    }

    // Handle replay playback mode (S-06)
    if (!menuActive && worldReady && g_app.isPlayingReplay && !g_app.useUnifiedSimulation) {
        g_app.replayPlayer.update(g_app.deltaTime);
        ApplyReplayFrame(g_app.replayPlayer.getInterpolatedFrame());
    } else if (!menuActive && worldReady) {
        // Normal simulation mode

        if (g_app.useUnifiedSimulation && g_app.creatureManager) {
            auto creatureStart = std::chrono::high_resolution_clock::now();
            LogWorldDiag("UpdateUnifiedSimulation begin.");
            UpdateUnifiedSimulation(g_app.deltaTime);
            LogWorldDiag("UpdateUnifiedSimulation end.");
            auto creatureEnd = std::chrono::high_resolution_clock::now();
            g_app.timings.creatureUpdate = std::chrono::duration<float>(creatureEnd - creatureStart).count();

            const auto& stats = g_app.creatureManager->getStats();
            int herbivores = stats.byType[static_cast<size_t>(CreatureType::GRAZER)] +
                             stats.byType[static_cast<size_t>(CreatureType::BROWSER)] +
                             stats.byType[static_cast<size_t>(CreatureType::FRUGIVORE)];
            int carnivores = stats.byType[static_cast<size_t>(CreatureType::SMALL_PREDATOR)] +
                             stats.byType[static_cast<size_t>(CreatureType::OMNIVORE)] +
                             stats.byType[static_cast<size_t>(CreatureType::APEX_PREDATOR)] +
                             stats.byType[static_cast<size_t>(CreatureType::SCAVENGER)];
            int aquatic = stats.byDomain[static_cast<size_t>(Forge::CreatureDomain::WATER)];
            int flying = stats.byDomain[static_cast<size_t>(Forge::CreatureDomain::AIR)];

            g_app.gameplay.update(g_app.deltaTime, g_app.world.simulationTime, g_app.creatureManager.get());
            g_app.gameplay.updatePopulation(
                stats.alive,
                herbivores,
                carnivores,
                aquatic,
                flying
            );
            g_app.world.maxGeneration = std::max(g_app.world.maxGeneration,
                                                 static_cast<uint32_t>(stats.currentGeneration));

            Forge::SimulationStats simStats;
            simStats.dayCount = g_app.seasonManager.getCurrentDay();
            simStats.totalCreatures = stats.alive;
            simStats.maxGeneration = stats.currentGeneration;
            simStats.simulationTime = g_app.world.simulationTime;
            g_app.simulationOrchestrator.updateStats(simStats);
        } else {
            // Profile GPU steering dispatch
            std::vector<SteeringOutput> steeringResults;
            if (g_app.gpuSteeringEnabled && g_app.world.GetAliveCount() >= GPU_STEERING_THRESHOLD) {
                DispatchGPUSteering(steeringResults);
            }

            // Profile creature update
            auto creatureStart = std::chrono::high_resolution_clock::now();
            LogWorldDiag("UpdateSimulationWorld begin.");
            if (!steeringResults.empty()) {
                g_app.world.Update(g_app.deltaTime, &steeringResults);
            } else {
                g_app.world.Update(g_app.deltaTime, nullptr);
            }
            LogWorldDiag("UpdateSimulationWorld end.");
            auto creatureEnd = std::chrono::high_resolution_clock::now();
            g_app.timings.creatureUpdate = std::chrono::duration<float>(creatureEnd - creatureStart).count();

            // Profile replay recording (S-05)
            if (g_app.isRecording) {
                auto replayStart = std::chrono::high_resolution_clock::now();
                float scaledDt = g_app.deltaTime * g_app.world.timeScale;
                g_app.replayRecorder.update(scaledDt, g_app.world.simulationTime);
                Forge::ReplayFrame frame = BuildReplayFrame();
                g_app.replayRecorder.recordFrame(frame);
                auto replayEnd = std::chrono::high_resolution_clock::now();
                g_app.timings.replayRecording = std::chrono::duration<float>(replayEnd - replayStart).count();
            }

            // Update gameplay systems (Agent 26 - Fun Factor)
            // Note: GameplayManager needs a CreatureManager, but we use SimulationWorld
            g_app.gameplay.update(g_app.deltaTime, g_app.world.simulationTime, nullptr);

            // Update population stats for gameplay manager
            g_app.gameplay.updatePopulation(
                static_cast<int>(g_app.world.GetAliveCount()),
                static_cast<int>(g_app.world.herbivoreCount),
                static_cast<int>(g_app.world.carnivoreCount),
                0, // aquatic count (not tracked separately in SimulationWorld)
                0  // flying count (not tracked separately in SimulationWorld)
            );
        }

        // Update stress test (Agent 30 - Performance Testing)
        UpdateStressTest(g_app.deltaTime);
    }

    // Update camera position
    float yawRad = glm::radians(g_app.cameraYaw);
    float pitchRad = glm::radians(g_app.cameraPitch);

    if (g_app.mouseCaptured && !g_app.cameraTransition.active && !g_app.isPlayingReplay) {
        glm::vec3 forward;
        forward.x = std::sin(yawRad) * std::cos(pitchRad);
        forward.y = std::sin(pitchRad);
        forward.z = std::cos(yawRad) * std::cos(pitchRad);
        forward = glm::normalize(forward);

        g_app.cameraTarget = g_app.cameraPosition + forward;
        g_app.cameraDistance = glm::length(g_app.cameraTarget - g_app.cameraPosition);
    } else {
        g_app.cameraPosition.x = g_app.cameraTarget.x + g_app.cameraDistance * cos(pitchRad) * sin(yawRad);
        g_app.cameraPosition.y = g_app.cameraTarget.y + g_app.cameraDistance * sin(pitchRad);
        g_app.cameraPosition.z = g_app.cameraTarget.z + g_app.cameraDistance * cos(pitchRad) * cos(yawRad);
    }

    LogWorldDiag("UpdateFrame end.");
}

// ============================================================================
// Render Frame (with profiling)
// ============================================================================
void RenderFrame() {
    auto renderStart = std::chrono::high_resolution_clock::now();
    const bool diagActive = g_app.worldDiagnostics && g_app.worldDiagnosticsFrames > 0;
    if (diagActive) {
        LogWorldDiag("RenderFrame begin.");
    }

    // Wait for previous frame
    g_app.frameFence->Wait(g_app.fenceValue);

    // Begin frame
    g_app.device->BeginFrame();
    g_app.commandList->Begin();

    // Get current backbuffer
    ITexture* backbuffer = g_app.swapchain->GetCurrentBackbuffer();

    // Transition to render target
    g_app.commandList->ResourceBarrier(backbuffer, ResourceState::Present, ResourceState::RenderTarget);

    // Transition depth buffer to depth write state if needed (first frame starts in Common state)
    static bool firstFrame = true;
    if (firstFrame) {
        g_app.commandList->ResourceBarrier(g_app.depthBuffer.get(), ResourceState::Common, ResourceState::DepthWrite);
        firstFrame = false;
    }

    SkyColors sky = g_app.dayNight.GetSkyColors();
    glm::vec3 lightDir = glm::normalize(glm::vec3(
        g_app.dayNight.GetLightDirection().x,
        g_app.dayNight.GetLightDirection().y,
        g_app.dayNight.GetLightDirection().z
    ));
    glm::vec3 lightColor = glm::vec3(sky.sunColor.x, sky.sunColor.y, sky.sunColor.z) * sky.sunIntensity;

    // Clear render target
    Math::Vec4 clearColor(sky.skyTop.x, sky.skyTop.y, sky.skyTop.z, 1.0f);
    g_app.commandList->ClearRenderTarget(backbuffer, clearColor);
    g_app.commandList->ClearDepthStencil(g_app.depthBuffer.get(), 1.0f, 0);

    // Set render targets
    ITexture* rts[] = { backbuffer };
    g_app.commandList->SetRenderTargets(Span<ITexture*>(rts, 1), g_app.depthBuffer.get());

    // Set viewport and scissor
    Viewport viewport = { 0.0f, 0.0f, static_cast<f32>(g_app.window->GetWidth()),
                          static_cast<f32>(g_app.window->GetHeight()), 0.0f, 1.0f };
    Scissor scissor = { 0, 0, g_app.window->GetWidth(), g_app.window->GetHeight() };
    g_app.commandList->SetViewport(viewport);
    g_app.commandList->SetScissor(scissor);

    float aspectRatio = static_cast<float>(g_app.window->GetWidth()) /
                        static_cast<float>(g_app.window->GetHeight());
    glm::mat4 sceneView = glm::lookAt(g_app.cameraPosition, g_app.cameraTarget, glm::vec3(0, 1, 0));
    glm::mat4 sceneProj = glm::perspectiveRH_ZO(glm::radians(CAMERA_FOV_DEGREES), aspectRatio, 0.1f, 5000.0f);
    glm::mat4 sceneViewProj = sceneProj * sceneView;

    const bool menuActive = g_app.mainMenu.isActive();
    const bool worldReady = g_app.hasGeneratedWorld && !g_app.isLoading;
    const bool renderWorld = worldReady && !menuActive;
    const bool renderGameUI = worldReady && !menuActive;

    if (diagActive) {
        LogWorldDiag(std::string("RenderFrame state: renderWorld=") +
                     (renderWorld ? "true" : "false") +
                     " renderUI=" + (renderGameUI ? "true" : "false"));
    }

    if (renderWorld) {
        // Render 3D scene - terrain first (replaces ground plane when enabled)
        if (g_app.terrainRenderingEnabled && g_app.terrainRenderer && g_app.terrainRenderer->isInitialized() && g_app.terrainPipeline) {
            // Get elapsed time for animation
            static auto terrainStartTime = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            float terrainTime = std::chrono::duration<float>(now - terrainStartTime).count();

            // Render terrain
            if (diagActive) {
                LogWorldDiag("RenderFrame terrain begin.");
            }
            g_app.terrainRenderer->render(
                g_app.commandList.get(),
                g_app.terrainPipeline.get(),
                sceneView,
                sceneProj,
                g_app.cameraPosition,
                lightDir,
                lightColor,
                terrainTime
            );
            if (diagActive) {
                LogWorldDiag("RenderFrame terrain end.");
            }
        } else {
            // Fallback: Simple ground plane if terrain not available
            if (diagActive) {
                LogWorldDiag("RenderFrame terrain fallback.");
            }
            RenderGroundPlane(sceneViewProj, g_app.cameraPosition, lightDir, lightColor);
        }

        // Render grass (after terrain, before water and creatures)
        if (g_app.grassRenderingEnabled && g_app.grassRenderer && g_app.grassPipeline) {
            static auto grassStartTime = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            float grassTime = std::chrono::duration<float>(now - grassStartTime).count();

            // Update visible grass instances based on camera position
            if (diagActive) {
                LogWorldDiag("RenderFrame grass update begin.");
            }
            g_app.grassRenderer->updateInstances(g_app.cameraPosition);
            if (diagActive) {
                LogWorldDiag("RenderFrame grass update end.");
            }

            // Render grass
            if (diagActive) {
                LogWorldDiag("RenderFrame grass render begin.");
            }
            g_app.grassRenderer->render(
                g_app.commandList.get(),
                g_app.grassPipeline.get(),
                sceneViewProj,
                g_app.cameraPosition,
                lightDir,
                lightColor,
                grassTime
            );
            if (diagActive) {
                LogWorldDiag("RenderFrame grass render end.");
            }
        }

        // Render trees (after grass, before water and creatures)
        if (g_app.treeRenderingEnabled && g_app.treeRenderer && g_app.treePipeline) {
            static auto treeStartTime = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            float treeTime = std::chrono::duration<float>(now - treeStartTime).count();

            // Render trees
            if (diagActive) {
                LogWorldDiag("RenderFrame trees begin.");
            }
            g_app.treeRenderer->render(
                g_app.commandList.get(),
                g_app.treePipeline.get(),
                sceneViewProj,
                g_app.cameraPosition,
                lightDir,
                lightColor,
                treeTime
            );
            if (diagActive) {
                LogWorldDiag("RenderFrame trees end.");
            }
        }

        // Render water (after terrain, grass, and trees, before creatures)
        if (g_app.waterRenderingEnabled && g_app.waterRenderer.IsInitialized()) {
            // Calculate elapsed time for water animation
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            float elapsedTime = std::chrono::duration<float>(now - startTime).count();

            // Light direction (sun direction, pointing towards light source)
            float sunIntensity = sky.sunIntensity;

            if (diagActive) {
                LogWorldDiag("RenderFrame water begin.");
            }
            g_app.waterRenderer.Render(
                g_app.commandList.get(),
                sceneView,
                sceneProj,
                g_app.cameraPosition,
                lightDir,
                lightColor,
                sunIntensity,
                elapsedTime
            );
            if (diagActive) {
                LogWorldDiag("RenderFrame water end.");
            }
        }

        if (diagActive) {
            LogWorldDiag("RenderFrame creatures begin.");
        }
        RenderCreatures(sceneViewProj, g_app.cameraPosition, lightDir, lightColor);
        if (diagActive) {
            LogWorldDiag("RenderFrame creatures end.");
        }
    }

    // Measure main rendering time
    auto renderingEnd = std::chrono::high_resolution_clock::now();
    g_app.timings.rendering = std::chrono::duration<float>(renderingEnd - renderStart).count();

    // Render ImGui with profiling
    if (g_app.imguiInitialized) {
        auto uiStart = std::chrono::high_resolution_clock::now();

        if (diagActive) {
            LogWorldDiag("RenderFrame ImGui begin.");
        }
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Phase 10 Agent 8: Update camera object to match orbit camera
        g_app.camera.Position = g_app.cameraPosition;
        g_app.camera.Yaw = g_app.cameraYaw;
        g_app.camera.Pitch = g_app.cameraPitch;
        g_app.camera.updateCameraVectors();

        // Phase 10 Agent 8: Update selection system
        if (renderGameUI && g_app.useUnifiedSimulation && g_app.creatureManager) {
            g_app.selectionSystem.update(g_app.camera, *g_app.creatureManager,
                                        (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
        }

        if (renderGameUI && g_app.godModeEnabled) {
            ImGuiIO& io = ImGui::GetIO();
            g_app.godModeUI.setEnabled(true);
            g_app.godModeUI.setScreenSize(io.DisplaySize.x, io.DisplaySize.y);
            g_app.godModeUI.update(g_app.deltaTime);
            g_app.godModeUI.render();
            g_app.godModeUI.renderOverlays();
        } else {
            g_app.godModeUI.setEnabled(false);
        }

        if (renderGameUI) {
            RenderCreatureNametags(sceneViewProj, g_app.cameraPosition);

            // Debug: Always show a simple window to verify ImGui works
            ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
            ImGui::Begin("Status");
            ImGui::Text("Simulation Running!");
            int creatureCount = g_app.useUnifiedSimulation && g_app.creatureManager
                ? g_app.creatureManager->getTotalPopulation()
                : static_cast<int>(g_app.world.GetAliveCount());
            ImGui::Text("Creatures: %d", creatureCount);
            ImGui::Text("FPS: %.1f", g_app.fps);
            ImGui::Text("Press F1 for full debug panel");
            ImGui::End();

            RenderDebugPanel();
            RenderPerformanceOverlay();
            RenderMinimalFPSOverlay();  // Agent 30: Always-visible FPS overlay
            RenderCameraControlOverlay();  // PHASE 11 Agent 10: Camera mode indicator
            RenderHelpOverlay();
            RenderCreatureInfoPanel();
            RenderCameraSettingsPanel();
        }
        RenderLoadingScreen();

        // Render gameplay UI (Agent 26 - Fun Factor)
        if (renderGameUI) {
            ImGuiIO& io = ImGui::GetIO();
            // Sync pause state between world and gameplay manager
            g_app.gameplay.setPaused(g_app.world.paused);
            g_app.gameplayUI.render(g_app.gameplay, io.DisplaySize.x, io.DisplaySize.y);
            // Handle input (may modify pause state)
            g_app.gameplayUI.handleInput(g_app.gameplay);
            // Sync back
            g_app.world.paused = g_app.gameplay.isPaused();
        }

        // Render notifications
        if (renderGameUI) {
            ImGuiIO& io = ImGui::GetIO();
            g_app.notifications.render(io.DisplaySize.x);
        }

        // Phase 10 Agent 8: Render inspection panel and selection indicators
        if (renderGameUI && g_app.useUnifiedSimulation) {
            g_app.inspectionPanel.render();
            g_app.selectionSystem.renderSelectionIndicators(g_app.camera, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
            g_app.inspectionPanel.renderScreenIndicator(g_app.camera, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
        }

        if (g_app.mainMenu.isActive()) {
            g_app.mainMenu.render();
        }

        ImGui::Render();

        ID3D12GraphicsCommandList* cmdList = static_cast<ID3D12GraphicsCommandList*>(
            g_app.commandList->GetNativeCommandList()
        );
        cmdList->SetDescriptorHeaps(1, g_app.imguiSrvHeap.GetAddressOf());
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

        auto uiEnd = std::chrono::high_resolution_clock::now();
        g_app.timings.uiRendering = std::chrono::duration<float>(uiEnd - uiStart).count();
        if (diagActive) {
            LogWorldDiag("RenderFrame ImGui end.");
        }
    }

    // Transition to present
    g_app.commandList->ResourceBarrier(backbuffer, ResourceState::RenderTarget, ResourceState::Present);

    // End command list and submit
    g_app.commandList->End();
    g_app.device->Submit(g_app.commandList.get());

    // Signal fence
    g_app.fenceValue++;
    g_app.device->SignalFence(g_app.frameFence.get(), g_app.fenceValue);

    // Present
    g_app.swapchain->Present();

    g_app.device->EndFrame();

    // Calculate total frame time and update history
    auto frameEnd = std::chrono::high_resolution_clock::now();
    g_app.timings.total = std::chrono::duration<float>(frameEnd - renderStart).count();
    g_app.timings.pushHistory();

    if (g_app.worldDiagnostics && g_app.worldDiagnosticsFrames > 0) {
        g_app.worldDiagnosticsFrames--;
        if (g_app.worldDiagnosticsFrames == 0) {
            AppendRuntimeDiagLog("Diagnostics complete.");
            g_app.worldDiagnostics = false;
        }
    }
}

// ============================================================================
// Handle Input
// ============================================================================
void HandleInput() {
    bool blockKeyboard = false;
    bool blockMouse = false;
    if (g_app.imguiInitialized && ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        blockKeyboard = io.WantCaptureKeyboard;
        blockMouse = io.WantCaptureMouse;
    }

    static bool leftPressed = false;
    bool leftDown = g_app.window->IsMouseButtonDown(MouseButton::Left);
    if (!blockMouse && leftDown && !leftPressed) {
        g_app.mouseCaptured = !g_app.mouseCaptured;
        g_app.window->SetCursorLocked(g_app.mouseCaptured);
    }
    leftPressed = leftDown;

    if (!blockKeyboard && g_app.mouseCaptured && g_app.window->IsKeyDown(KeyCode::Escape)) {
        g_app.mouseCaptured = false;
        g_app.window->SetCursorLocked(false);
    }

    // Toggle debug panel (F1)
    static bool f1Pressed = false;
    bool f1Down = g_app.window->IsKeyDown(KeyCode::F1);
    if (!blockKeyboard && f1Down) {
        if (!f1Pressed) {
            g_app.showDebugPanel = !g_app.showDebugPanel;
        }
    }
    f1Pressed = f1Down;

    // Toggle performance overlay (F2)
    static bool f2Pressed = false;
    bool f2Down = g_app.window->IsKeyDown(KeyCode::F2);
    if (!blockKeyboard && f2Down) {
        if (!f2Pressed) {
            g_app.showPerformanceOverlay = !g_app.showPerformanceOverlay;
        }
    }
    f2Pressed = f2Down;

    // Toggle help overlay (F3)
    static bool f3Pressed = false;
    bool f3Down = g_app.window->IsKeyDown(KeyCode::F3);
    if (!blockKeyboard && f3Down) {
        if (!f3Pressed) {
            g_app.showHelpOverlay = !g_app.showHelpOverlay;
        }
    }
    f3Pressed = f3Down;

    // Toggle pause (P)
    static bool pPressed = false;
    bool pDown = g_app.window->IsKeyDown(KeyCode::P);
    if (!blockKeyboard && pDown) {
        if (!pPressed) {
            g_app.world.paused = !g_app.world.paused;
        }
    }
    pPressed = pDown;

    // Quick Save (F5)
    static bool f5Pressed = false;
    bool f5Down = g_app.window->IsKeyDown(KeyCode::F5);
    if (!blockKeyboard && f5Down) {
        if (!f5Pressed) {
            QuickSave();
        }
    }
    f5Pressed = f5Down;

    // Quick Load (F9)
    static bool f9Pressed = false;
    bool f9Down = g_app.window->IsKeyDown(KeyCode::F9);
    if (!blockKeyboard && f9Down) {
        if (!f9Pressed) {
            QuickLoad();
        }
    }
    f9Pressed = f9Down;

    // Toggle replay mode (F10) - S-05, S-06
    static bool f10Pressed = false;
    bool f10Down = g_app.window->IsKeyDown(KeyCode::F10);
    if (!blockKeyboard && f10Down) {
        if (!f10Pressed) {
            if (g_app.useUnifiedSimulation) {
                g_app.statusMessage = "Replay disabled in unified simulation";
                g_app.statusMessageTimer = 2.0f;
            } else {
                if (g_app.isPlayingReplay) {
                    ExitReplayMode();
                } else {
                    EnterReplayMode();
                }
            }
        }
    }
    f10Pressed = f10Down;

    // Replay playback controls
    if (g_app.isPlayingReplay) {
        // Space to toggle play/pause
        static bool spacePressed = false;
        bool spaceDown = g_app.window->IsKeyDown(KeyCode::Space);
        if (!blockKeyboard && spaceDown) {
            if (!spacePressed) {
                g_app.replayPlayer.togglePause();
            }
        }
        spacePressed = spaceDown;

        // Arrow keys for frame stepping
        static bool leftPressed = false;
        static bool rightPressed = false;
        bool leftDown = g_app.window->IsKeyDown(KeyCode::Left);
        if (!blockKeyboard && leftDown) {
            if (!leftPressed) {
                g_app.replayPlayer.stepBackward();
            }
        }
        leftPressed = leftDown;
        bool rightDown = g_app.window->IsKeyDown(KeyCode::Right);
        if (!blockKeyboard && rightDown) {
            if (!rightPressed) {
                g_app.replayPlayer.stepForward();
            }
        }
        rightPressed = rightDown;

        // Skip normal camera controls during replay
        return;
    }

    // Camera reset (R key) - PHASE 11 Agent 10
    static bool rKeyPressed = false;
    if (!blockKeyboard) {
        bool rKeyDown = g_app.window->IsKeyDown(KeyCode::R);
        if (rKeyDown && !rKeyPressed) {
            // Reset to default camera position and orientation
            g_app.cameraPosition = glm::vec3(0.0f, 100.0f, 200.0f);
            g_app.cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
            g_app.cameraYaw = 0.0f;
            g_app.cameraPitch = 30.0f;
            g_app.cameraDistance = 200.0f;
            g_app.mouseCaptured = false;
            g_app.window->SetCursorLocked(false);
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
        }
        rKeyPressed = rKeyDown;
    }

    // Camera controls
    if (g_app.mouseCaptured) {
        // FPS mode (mouse captured) - WASD moves camera
        if (!blockKeyboard) {
            float moveSpeed = g_app.cameraMoveSpeed * g_app.deltaTime;

            float yawRad = glm::radians(g_app.cameraYaw);
            float pitchRad = glm::radians(g_app.cameraPitch);
            glm::vec3 forward(
                std::sin(yawRad) * std::cos(pitchRad),
                std::sin(pitchRad),
                std::cos(yawRad) * std::cos(pitchRad)
            );
            forward = glm::normalize(forward);
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

            if (g_app.window->IsKeyDown(KeyCode::W)) {
                g_app.cameraPosition += forward * moveSpeed;
            }
            if (g_app.window->IsKeyDown(KeyCode::S)) {
                g_app.cameraPosition -= forward * moveSpeed;
            }
            if (g_app.window->IsKeyDown(KeyCode::A)) {
                g_app.cameraPosition -= right * moveSpeed;
            }
            if (g_app.window->IsKeyDown(KeyCode::D)) {
                g_app.cameraPosition += right * moveSpeed;
            }
        }

        // Mouse look (FPS mode)
        if (!blockMouse) {
            Math::Vec2 delta = g_app.window->GetMouseDelta();
            // Apply mouse inversion settings (default: move mouse right = look right)
            float xMult = g_app.invertMouseX ? 1.0f : -1.0f;
            float yMult = g_app.invertMouseY ? 1.0f : -1.0f;
            g_app.cameraYaw += delta.x * g_app.mouseSensitivity * xMult;
            g_app.cameraPitch += delta.y * g_app.mouseSensitivity * yMult;
            // Clamp pitch to prevent gimbal lock
            g_app.cameraPitch = glm::clamp(g_app.cameraPitch, -89.0f, 89.0f);
        }
    } else {
        // Not in FPS mode - check for follow mode first
        if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) {
            // In follow mode, WASD adjusts orbit angle and distance
            if (!blockKeyboard) {
                float orbitSpeed = 90.0f * g_app.deltaTime;  // degrees per second
                if (g_app.window->IsKeyDown(KeyCode::A)) {
                    g_app.followOrbitAngle -= orbitSpeed;
                }
                if (g_app.window->IsKeyDown(KeyCode::D)) {
                    g_app.followOrbitAngle += orbitSpeed;
                }
                if (g_app.window->IsKeyDown(KeyCode::W)) {
                    g_app.followDistance = std::max(5.0f, g_app.followDistance - 30.0f * g_app.deltaTime);
                }
                if (g_app.window->IsKeyDown(KeyCode::S)) {
                    g_app.followDistance = std::min(100.0f, g_app.followDistance + 30.0f * g_app.deltaTime);
                }
                if (g_app.window->IsKeyDown(KeyCode::Q)) {
                    g_app.followHeight = std::max(2.0f, g_app.followHeight - 20.0f * g_app.deltaTime);
                }
                if (g_app.window->IsKeyDown(KeyCode::E)) {
                    g_app.followHeight = std::min(50.0f, g_app.followHeight + 20.0f * g_app.deltaTime);
                }
            }
        } else {
            // Free orbit mode - WASD moves camera target point
            if (!blockKeyboard) {
                float moveSpeed = g_app.cameraMoveSpeed * g_app.deltaTime;

                if (g_app.window->IsKeyDown(KeyCode::W)) {
                    g_app.cameraTarget.z -= moveSpeed;
                }
                if (g_app.window->IsKeyDown(KeyCode::S)) {
                    g_app.cameraTarget.z += moveSpeed;
                }
                if (g_app.window->IsKeyDown(KeyCode::A)) {
                    g_app.cameraTarget.x -= moveSpeed;
                }
                if (g_app.window->IsKeyDown(KeyCode::D)) {
                    g_app.cameraTarget.x += moveSpeed;
                }
            }
        }

        // Mouse camera rotation (when right button held in orbit mode)
        if (!blockMouse && g_app.window->IsMouseButtonDown(MouseButton::Right)) {
            Math::Vec2 delta = g_app.window->GetMouseDelta();
            // Apply mouse inversion settings
            float xMult = g_app.invertMouseX ? 1.0f : -1.0f;
            float yMult = g_app.invertMouseY ? 1.0f : -1.0f;
            // Use same sensitivity as FPS mode for consistency
            g_app.cameraYaw += delta.x * g_app.mouseSensitivity * xMult;
            g_app.cameraPitch += delta.y * g_app.mouseSensitivity * yMult;
            // Orbit mode pitch range (10 to 89 to avoid looking straight down/up)
            g_app.cameraPitch = glm::clamp(g_app.cameraPitch, 10.0f, 89.0f);
        }

        // Mouse scroll zoom
        if (!blockMouse) {
            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseWheel != 0.0f) {
                float zoomDelta = io.MouseWheel * g_app.zoomSpeed;
                g_app.cameraDistance = glm::clamp(g_app.cameraDistance - zoomDelta,
                                                   g_app.minZoom, g_app.maxZoom);
            }
        }
    }

    // Camera preset hotkeys (1, 2, 3)
    static bool key1Pressed = false;
    static bool key2Pressed = false;
    static bool key3Pressed = false;

    if (!blockKeyboard) {
        bool key1Down = g_app.window->IsKeyDown(KeyCode::Num1);
        bool key2Down = g_app.window->IsKeyDown(KeyCode::Num2);
        bool key3Down = g_app.window->IsKeyDown(KeyCode::Num3);

        if (key1Down && !key1Pressed) {
            // Overview mode - high above, sees whole world
            g_app.currentPreset = AppState::CameraPreset::OVERVIEW;
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            StartCameraTransition(
                glm::vec3(0.0f, 300.0f, 50.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                1.5f
            );
        }
        if (key2Down && !key2Pressed) {
            // Ground level - creature eye view
            g_app.currentPreset = AppState::CameraPreset::GROUND;
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            StartCameraTransition(
                glm::vec3(g_app.cameraTarget.x, 5.0f, g_app.cameraTarget.z + 30.0f),
                glm::vec3(g_app.cameraTarget.x, 3.0f, g_app.cameraTarget.z),
                1.0f
            );
        }
        if (key3Down && !key3Pressed) {
            // Cinematic - slow pan
            g_app.currentPreset = AppState::CameraPreset::CINEMATIC;
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            g_app.cinematicPlaying = true;
            g_app.cinematicTime = 0.0f;
        }

        key1Pressed = key1Down;
        key2Pressed = key2Down;
        key3Pressed = key3Down;
    }

    // Follow mode toggle (F key)
    static bool fKeyPressed = false;
    if (!blockKeyboard) {
        bool fKeyDown = g_app.window->IsKeyDown(KeyCode::F);
        if (fKeyDown && !fKeyPressed) {
            if (g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) {
                // Exit follow mode
                g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
                g_app.followCreatureId = -1;
                g_app.followCreature = nullptr;
            } else if (g_app.useUnifiedSimulation) {
                Creature* selected = g_app.selectionSystem.getSelectedCreature();
                if (selected) {
                    // Enter follow mode with selected creature
                    g_app.cameraFollowMode = AppState::CameraFollowMode::FOLLOW;
                    g_app.followCreature = selected;
                    g_app.followCreatureId = -1;
                    g_app.followOrbitAngle = g_app.cameraYaw;
                }
            } else if (g_app.selectedCreatureIndex >= 0) {
                // Enter follow mode with selected creature
                g_app.cameraFollowMode = AppState::CameraFollowMode::FOLLOW;
                g_app.followCreatureId = g_app.selectedCreatureIndex;
                g_app.followOrbitAngle = g_app.cameraYaw;
            }
        }
        fKeyPressed = fKeyDown;
    }

    // ESC exits follow mode
    if (!blockKeyboard && g_app.cameraFollowMode != AppState::CameraFollowMode::NONE) {
        if (g_app.window->IsKeyDown(KeyCode::Escape)) {
            g_app.cameraFollowMode = AppState::CameraFollowMode::NONE;
            g_app.followCreatureId = -1;
            g_app.followCreature = nullptr;
            g_app.cinematicPlaying = false;
        }
    }
}

// ============================================================================
// Cleanup
// ============================================================================
void Cleanup() {
    if (g_app.worldGenFuture.valid()) {
        g_app.worldGenFuture.wait();
    }

    // Wait for GPU to finish
    if (g_app.device) {
        g_app.device->WaitIdle();
    }

    // Shutdown GPU steering
    if (g_app.gpuSteering) {
        g_app.gpuSteering->Shutdown();
        g_app.gpuSteering.reset();
    }

    // Shutdown ImGui
    if (g_app.imguiInitialized) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    // Release resources (unique_ptr handles most cleanup)
    g_app.imguiSrvHeap.Reset();
}

// ============================================================================
// Main Entry Point
// ============================================================================
int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "    OrganismEvolution - Evolution Simulator       " << std::endl;
    std::cout << "    DirectX 12 Build with GPU Compute             " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;

    // Create window
    std::cout << "Creating window..." << std::endl;
    WindowConfig windowConfig;
    windowConfig.title = "OrganismEvolution - DX12";
    windowConfig.width = WINDOW_WIDTH;
    windowConfig.height = WINDOW_HEIGHT;
    windowConfig.resizable = true;

    g_app.window = IWindow::Create(windowConfig);
    if (!g_app.window) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }

    // Create graphics device
    std::cout << "Creating DirectX 12 device..." << std::endl;
    DeviceConfig deviceConfig;
    deviceConfig.api = GraphicsAPI::DirectX12;
    deviceConfig.enableValidation = true;
    deviceConfig.enableGPUValidation = false;

    g_app.device = CreateDevice(deviceConfig);
    if (!g_app.device) {
        std::cerr << "Failed to create DX12 device!" << std::endl;
        return -1;
    }

    std::cout << "  API: DirectX 12" << std::endl;
    std::cout << "  Device: " << g_app.device->GetCapabilities().deviceName << std::endl;

    // Create swapchain
    std::cout << "Creating swapchain..." << std::endl;
    SwapchainDesc swapchainDesc;
    swapchainDesc.windowHandle = g_app.window->GetNativeHandle();
    swapchainDesc.width = WINDOW_WIDTH;
    swapchainDesc.height = WINDOW_HEIGHT;
    swapchainDesc.bufferCount = FRAME_COUNT;
    swapchainDesc.format = Format::R8G8B8A8_UNORM;
    swapchainDesc.vsync = true;

    g_app.swapchain = g_app.device->CreateSwapchain(swapchainDesc);
    if (!g_app.swapchain) {
        std::cerr << "Failed to create swapchain!" << std::endl;
        return -1;
    }

    // Create depth buffer
    std::cout << "Creating depth buffer..." << std::endl;
    TextureDesc depthDesc;
    depthDesc.width = WINDOW_WIDTH;
    depthDesc.height = WINDOW_HEIGHT;
    depthDesc.depth = 1;
    depthDesc.format = Format::D32_FLOAT;
    depthDesc.type = TextureType::Texture2D;
    depthDesc.usage = TextureUsage::DepthStencil;

    g_app.depthBuffer = g_app.device->CreateTexture(depthDesc);
    if (!g_app.depthBuffer) {
        std::cerr << "Failed to create depth buffer!" << std::endl;
        return -1;
    }

    // Create command list
    g_app.commandList = g_app.device->CreateCommandList();
    if (!g_app.commandList) {
        std::cerr << "Failed to create command list!" << std::endl;
        return -1;
    }

    // Create compute command list for GPU steering
    g_app.computeCommandList = g_app.device->CreateCommandList(CommandListType::Compute);
    if (!g_app.computeCommandList) {
        std::cerr << "Failed to create compute command list!" << std::endl;
        return -1;
    }

    // Create fence
    g_app.frameFence = g_app.device->CreateFence(0);
    if (!g_app.frameFence) {
        std::cerr << "Failed to create fence!" << std::endl;
        return -1;
    }

    // Create compute fence for GPU readback sync
    g_app.computeFence = g_app.device->CreateFence(0);
    if (!g_app.computeFence) {
        std::cerr << "Failed to create compute fence!" << std::endl;
        return -1;
    }

    // Initialize ImGui
    std::cout << "Initializing ImGui..." << std::endl;
    if (!InitializeImGui()) {
        std::cerr << "Failed to initialize ImGui!" << std::endl;
        Cleanup();
        return -1;
    }

    // Initialize GPU Steering Compute
    // Agent 36: Attempting GPU steering with proper error capture
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "    GPU STEERING COMPUTE STATUS\n";
    std::cout << "========================================\n";

    bool gpuSteeringSuccess = false;
    std::string gpuSteeringError = "";

    try {
        gpuSteeringSuccess = InitializeGPUSteering();
        if (!gpuSteeringSuccess) {
            gpuSteeringError = "InitializeGPUSteering() returned false";
        }
    } catch (const std::exception& e) {
        gpuSteeringError = std::string("Exception: ") + e.what();
        gpuSteeringSuccess = false;
    } catch (...) {
        gpuSteeringError = "Unknown exception during GPU steering init";
        gpuSteeringSuccess = false;
    }

    if (gpuSteeringSuccess) {
        const bool enableGpuSteeringByDefault = false;
        std::cout << "[GPU STEERING] *** INITIALIZED ***\n";
        std::cout << "[GPU STEERING] Compute shader: Runtime/Shaders/SteeringCompute.hlsl\n";
        std::cout << "[GPU STEERING] Max creatures: " << GPUSteeringCompute::MAX_CREATURES << "\n";
        std::cout << "[GPU STEERING] Thread group size: " << GPUSteeringCompute::THREAD_GROUP_SIZE << "\n";
        std::cout << "[GPU STEERING] Threshold: " << GPU_STEERING_THRESHOLD << " creatures\n";
        if (enableGpuSteeringByDefault) {
            std::cout << "[GPU STEERING] Status: ENABLED\n";
        } else {
            std::cout << "[GPU STEERING] Status: DISABLED (enable via debug panel)\n";
        }
        g_app.gpuSteeringEnabled = enableGpuSteeringByDefault;
        g_app.gpuSteeringAvailable = true;
    } else {
        std::cout << "[GPU STEERING] *** DISABLED - USING CPU FALLBACK ***\n";
        std::cout << "[GPU STEERING] Reason: " << gpuSteeringError << "\n";
        std::cout << "[GPU STEERING] This is a silent performance limitation!\n";
        std::cout << "[GPU STEERING] Large populations (>200) will be CPU-bound.\n";
        g_app.gpuSteeringEnabled = false;
        g_app.gpuSteeringAvailable = false;
    }
    std::cout << "========================================\n\n";

    // Initialize creature 3D rendering
    if (!InitializeCreatureRendering()) {
        std::cerr << "WARNING: Creature rendering failed to initialize!" << std::endl;
        std::cerr << "  Creatures will not be visible, but simulation will run." << std::endl;
    }

    // Initialize ground plane (test basic rendering)
    if (!InitializeGroundPlane()) {
        std::cerr << "WARNING: Ground plane failed to initialize!" << std::endl;
    }

    // Initialize water rendering system (Phase 5 - 3D World)
    std::cout << "Initializing water rendering..." << std::endl;
    if (g_app.waterRenderer.Initialize(g_app.device.get(),
                                        Format::R8G8B8A8_UNORM,
                                        Format::D32_FLOAT)) {
        // Generate water plane mesh (64x64 grid, 1000 units wide, at height -5)
        g_app.waterLevel = -5.0f;
        g_app.waterRenderer.GenerateMesh(64, 1000.0f, g_app.waterLevel);

        // Configure water appearance
        g_app.waterRenderer.SetWaterColor(
            glm::vec4(0.0f, 0.15f, 0.3f, 1.0f),   // Deep water color
            glm::vec4(0.1f, 0.4f, 0.5f, 1.0f)     // Shallow water color
        );
        g_app.waterRenderer.SetWaveParams(0.08f, 0.3f, 0.8f);  // scale, speed, height
        g_app.waterRenderer.SetTransparency(0.75f);
        g_app.waterRenderer.SetFoamParams(0.2f, 8.0f);
        g_app.waterRenderer.SetSpecularParams(128.0f, 1.5f);
        g_app.waterRenderer.SetSkyColors(
            glm::vec3(0.4f, 0.6f, 0.95f),   // Sky top
            glm::vec3(0.7f, 0.8f, 0.95f)    // Sky horizon
        );

        std::cout << "  Water rendering initialized successfully" << std::endl;
        std::cout << "  Water level: " << g_app.waterLevel << std::endl;
        std::cout << "  Vertices: " << g_app.waterRenderer.GetVertexCount() << std::endl;
        std::cout << "  Indices: " << g_app.waterRenderer.GetIndexCount() << std::endl;
        // Disable separate water renderer - terrain shader now handles water rendering
        g_app.waterRenderingEnabled = false;
    } else {
        std::cerr << "WARNING: Water rendering failed to initialize!" << std::endl;
        g_app.waterRenderingEnabled = false;
    }

    // Initialize grass rendering system (Phase 5 - 3D World)
    std::cout << "Initializing grass rendering..." << std::endl;
    {
        // Create grass system
        g_app.grassSystem = std::make_unique<GrassSystem>();
        // Note: GrassSystem needs DX12Device and Terrain references for full initialization
        // For now, we'll just create the renderer

        // Grass vertex shader source (embedded)
        static const char* grassVSSource = R"HLSL(
// Grass Vertex Shader with GPU Instancing
cbuffer GrassConstants : register(b0) {
    float4x4 viewProj;
    float4 cameraPosUnused;
    float4 windDirTime;
    float4 baseColor;
    float4 tipColor;
    float4 lightDir;
    float4 lightColor;
    float4 lodParams;
};

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct InstanceData {
    float3 worldPos : INSTANCE_POSITION;
    float rotation : INSTANCE_ROTATION;
    float height : INSTANCE_HEIGHT;
    float width : INSTANCE_WIDTH;
    float bendFactor : INSTANCE_BEND;
    float colorVar : INSTANCE_COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 color : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

PSInput main(VSInput vertex, InstanceData instance) {
    PSInput output;

    float3 cameraPos = cameraPosUnused.xyz;
    float2 windDir = windDirTime.xy;
    float windStrength = windDirTime.z;
    float time = windDirTime.w;

    float3 toCam = normalize(cameraPos - instance.worldPos);
    float3 right = normalize(cross(float3(0, 1, 0), toCam));
    float3 up = float3(0, 1, 0);

    float c = cos(instance.rotation);
    float s = sin(instance.rotation);
    right = right * c + cross(up, right) * s;

    float windPhase = time * 2.0 + instance.worldPos.x * 0.5 + instance.worldPos.z * 0.3;
    float windOffset = sin(windPhase) * windStrength * vertex.texCoord.y * vertex.texCoord.y;
    float totalBend = windOffset + instance.bendFactor * 0.3;

    float3 localPos = right * vertex.position.x * instance.width +
                      up * vertex.position.y * instance.height;
    localPos.x += totalBend * vertex.texCoord.y * windDir.x;
    localPos.z += totalBend * vertex.texCoord.y * windDir.y;

    float3 worldPos = instance.worldPos + localPos;

    output.position = mul(viewProj, float4(worldPos, 1.0));
    output.texCoord = vertex.texCoord;
    output.worldPos = worldPos;

    float3 variedBase = baseColor.rgb * (1.0 - instance.colorVar * 0.2);
    float3 variedTip = tipColor.rgb * (1.0 + instance.colorVar * 0.1);
    output.color = lerp(variedBase, variedTip, vertex.texCoord.y);

    return output;
}
)HLSL";

        // Grass pixel shader source (embedded)
        static const char* grassPSSource = R"HLSL(
// Grass Pixel Shader
cbuffer GrassConstants : register(b0) {
    float4x4 viewProj;
    float4 cameraPosUnused;
    float4 windDirTime;
    float4 baseColor;
    float4 tipColor;
    float4 lightDir;
    float4 lightColor;
    float4 lodParams;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 color : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

float4 main(PSInput input) : SV_TARGET {
    float alpha = 1.0 - abs(input.texCoord.x * 2.0 - 1.0);
    alpha *= 1.0 - input.texCoord.y * 0.3;

    if (alpha < 0.5) discard;

    float3 L = normalize(-lightDir.xyz);
    float NdotL = max(dot(float3(0, 1, 0), L), 0.0);

    float3 ambient = input.color * 0.4;
    float3 diffuse = input.color * lightColor.rgb * NdotL * 0.6;
    float3 finalColor = ambient + diffuse;

    return float4(finalColor, 1.0);
}
)HLSL";

        // Create shaders
        ShaderDesc grassVSDesc;
        grassVSDesc.type = ShaderType::Vertex;
        grassVSDesc.source = grassVSSource;
        grassVSDesc.entryPoint = "main";
        grassVSDesc.debugName = "GrassVS";
        g_app.grassVertexShader = g_app.device->CreateShader(grassVSDesc);

        ShaderDesc grassPSDesc;
        grassPSDesc.type = ShaderType::Pixel;
        grassPSDesc.source = grassPSSource;
        grassPSDesc.entryPoint = "main";
        grassPSDesc.debugName = "GrassPS";
        g_app.grassPixelShader = g_app.device->CreateShader(grassPSDesc);

        if (g_app.grassVertexShader && g_app.grassPixelShader) {
            // Create grass pipeline
            PipelineDesc grassPipeDesc;
            grassPipeDesc.vertexShader = g_app.grassVertexShader.get();
            grassPipeDesc.pixelShader = g_app.grassPixelShader.get();

            // Per-vertex layout (slot 0)
            VertexAttribute posAttr;
            posAttr.semanticName = "POSITION";
            posAttr.semanticIndex = 0;
            posAttr.format = Format::R32G32B32_FLOAT;
            posAttr.inputSlot = 0;
            posAttr.offset = 0;
            posAttr.inputRate = InputRate::PerVertex;
            grassPipeDesc.vertexLayout.push_back(posAttr);

            VertexAttribute texAttr;
            texAttr.semanticName = "TEXCOORD";
            texAttr.semanticIndex = 0;
            texAttr.format = Format::R32G32_FLOAT;
            texAttr.inputSlot = 0;
            texAttr.offset = 12;
            texAttr.inputRate = InputRate::PerVertex;
            grassPipeDesc.vertexLayout.push_back(texAttr);

            // Per-instance layout (slot 1)
            VertexAttribute instPosAttr;
            instPosAttr.semanticName = "INSTANCE_POSITION";
            instPosAttr.semanticIndex = 0;
            instPosAttr.format = Format::R32G32B32_FLOAT;
            instPosAttr.inputSlot = 1;
            instPosAttr.offset = 0;
            instPosAttr.inputRate = InputRate::PerInstance;
            instPosAttr.instanceStepRate = 1;
            grassPipeDesc.vertexLayout.push_back(instPosAttr);

            VertexAttribute instRotAttr;
            instRotAttr.semanticName = "INSTANCE_ROTATION";
            instRotAttr.semanticIndex = 0;
            instRotAttr.format = Format::R32_FLOAT;
            instRotAttr.inputSlot = 1;
            instRotAttr.offset = 12;
            instRotAttr.inputRate = InputRate::PerInstance;
            instRotAttr.instanceStepRate = 1;
            grassPipeDesc.vertexLayout.push_back(instRotAttr);

            VertexAttribute instHeightAttr;
            instHeightAttr.semanticName = "INSTANCE_HEIGHT";
            instHeightAttr.semanticIndex = 0;
            instHeightAttr.format = Format::R32_FLOAT;
            instHeightAttr.inputSlot = 1;
            instHeightAttr.offset = 16;
            instHeightAttr.inputRate = InputRate::PerInstance;
            instHeightAttr.instanceStepRate = 1;
            grassPipeDesc.vertexLayout.push_back(instHeightAttr);

            VertexAttribute instWidthAttr;
            instWidthAttr.semanticName = "INSTANCE_WIDTH";
            instWidthAttr.semanticIndex = 0;
            instWidthAttr.format = Format::R32_FLOAT;
            instWidthAttr.inputSlot = 1;
            instWidthAttr.offset = 20;
            instWidthAttr.inputRate = InputRate::PerInstance;
            instWidthAttr.instanceStepRate = 1;
            grassPipeDesc.vertexLayout.push_back(instWidthAttr);

            VertexAttribute instBendAttr;
            instBendAttr.semanticName = "INSTANCE_BEND";
            instBendAttr.semanticIndex = 0;
            instBendAttr.format = Format::R32_FLOAT;
            instBendAttr.inputSlot = 1;
            instBendAttr.offset = 24;
            instBendAttr.inputRate = InputRate::PerInstance;
            instBendAttr.instanceStepRate = 1;
            grassPipeDesc.vertexLayout.push_back(instBendAttr);

            VertexAttribute instColorAttr;
            instColorAttr.semanticName = "INSTANCE_COLOR";
            instColorAttr.semanticIndex = 0;
            instColorAttr.format = Format::R32_FLOAT;
            instColorAttr.inputSlot = 1;
            instColorAttr.offset = 28;
            instColorAttr.inputRate = InputRate::PerInstance;
            instColorAttr.instanceStepRate = 1;
            grassPipeDesc.vertexLayout.push_back(instColorAttr);

            grassPipeDesc.primitiveTopology = PrimitiveTopology::TriangleList;
            grassPipeDesc.cullMode = CullMode::None;  // Double-sided grass
            grassPipeDesc.depthTestEnabled = true;
            grassPipeDesc.depthWriteEnabled = true;
            grassPipeDesc.depthCompareOp = CompareOp::Less;
            grassPipeDesc.blendEnabled = false;  // Alpha cutout, not blend
            grassPipeDesc.renderTargetFormats.push_back(Format::R8G8B8A8_UNORM);
            grassPipeDesc.depthStencilFormat = Format::D32_FLOAT;
            grassPipeDesc.debugName = "GrassPipeline";

            g_app.grassPipeline = g_app.device->CreatePipeline(grassPipeDesc);

            if (g_app.grassPipeline) {
                // Create grass renderer
                g_app.grassRenderer = std::make_unique<GrassRendererDX12>();
                if (g_app.grassRenderer->init(g_app.device.get(), g_app.grassSystem.get())) {
                    std::cout << "  Grass rendering initialized successfully" << std::endl;
                    g_app.grassRenderingEnabled = true;
                } else {
                    std::cerr << "  WARNING: Grass renderer init failed!" << std::endl;
                    g_app.grassRenderingEnabled = false;
                }
            } else {
                std::cerr << "  WARNING: Failed to create grass pipeline!" << std::endl;
                g_app.grassRenderingEnabled = false;
            }
        } else {
            std::cerr << "  WARNING: Failed to compile grass shaders!" << std::endl;
            g_app.grassRenderingEnabled = false;
        }
    }

    // Initialize tree rendering system (Phase 5 - 3D World)
    std::cout << "Initializing tree rendering..." << std::endl;
    {
        // Create terrain for vegetation placement (256x256, scale 2.0)
        g_app.terrain = std::make_unique<Terrain>(256, 256, 2.0f);
        g_app.terrain->generate(42);  // Use fixed seed
        float terrainWorldHalf = g_app.terrain->getWidth() * g_app.terrain->getScale() * 0.5f;
        g_app.world.SetWorldBounds(terrainWorldHalf);

        // Create vegetation manager
        g_app.vegetationManager = std::make_unique<VegetationManager>(g_app.terrain.get());
        g_app.vegetationManager->generate(42);  // Generate vegetation with same seed

        // Initialize aquatic plant system (Phase 8 - Ocean Ecosystem)
        g_app.vegetationManager->initializeAquaticPlants(nullptr, 42);  // DX12Device null, using Forge RHI
        if (g_app.vegetationManager->getAquaticPlants()) {
            auto aquaStats = g_app.vegetationManager->getAquaticStats();
            std::cout << "  Aquatic plants initialized:" << std::endl;
            std::cout << "    Kelp forests: " << aquaStats.totalKelpForests << std::endl;
            std::cout << "    Coral reefs: " << aquaStats.totalCoralReefs << std::endl;
            std::cout << "    Total aquatic plants: " << aquaStats.totalAquaticPlants << std::endl;
        }

        // Initialize grass system with terrain (grass system was created earlier, now bind terrain)
        if (g_app.grassSystem) {
            g_app.grassSystem->initialize(nullptr, g_app.terrain.get());  // DX12Device is null (using Forge RHI)
            g_app.grassSystem->generate(42);  // Generate grass instances with same seed
            std::cout << "  Grass instances generated: " << g_app.grassSystem->getInstances().size() << std::endl;
        }

        // Tree vertex shader source (embedded)
        static const char* treeVSSource = R"HLSL(
// Tree Vertex Shader
cbuffer TreeConstants : register(b0) {
    float4x4 viewProj;
    float4x4 model;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 windParams;  // xy=direction, z=strength, w=time
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 colorRG : TEXCOORD2;
};

PSInput main(VSInput input) {
    PSInput output;

    // Apply wind sway based on height
    float3 pos = input.position;
    float height = pos.y;
    float windPhase = windParams.w * 2.0 + pos.x * 0.5 + pos.z * 0.3;
    float windAmount = sin(windPhase) * windParams.z * height * 0.01;
    pos.x += windAmount * windParams.x;
    pos.z += windAmount * windParams.y;

    // Transform to world space
    float4 worldPos = mul(model, float4(pos, 1.0));
    output.position = mul(viewProj, worldPos);
    output.worldPos = worldPos.xyz;

    // Transform normal
    float3x3 normalMatrix = (float3x3)model;
    output.normal = normalize(mul(normalMatrix, input.normal));
    output.colorRG = input.texCoord;

    return output;
}
)HLSL";

        // Tree pixel shader source (embedded)
        static const char* treePSSource = R"HLSL(
// Tree Pixel Shader
cbuffer TreeConstants : register(b0) {
    float4x4 viewProj;
    float4x4 model;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 windParams;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 colorRG : TEXCOORD2;
};

float4 main(PSInput input) : SV_TARGET {
    // Reconstruct color
    float3 baseColor = float3(input.colorRG.x, input.colorRG.y, 0.0);
    baseColor.b = min(input.colorRG.x, input.colorRG.y) * 0.5;

    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir.xyz);
    float NdotL = max(dot(N, L), 0.0);

    float3 ambient = baseColor * 0.35;
    float3 diffuse = baseColor * lightColor.rgb * NdotL * 0.65;

    return float4(ambient + diffuse, 1.0);
}
)HLSL";

        // Compile tree shaders
        ShaderDesc treeVSDesc;
        treeVSDesc.type = ShaderType::Vertex;
        treeVSDesc.source = treeVSSource;
        treeVSDesc.entryPoint = "main";
        treeVSDesc.debugName = "TreeVS";

        ShaderDesc treePSDesc;
        treePSDesc.type = ShaderType::Pixel;
        treePSDesc.source = treePSSource;
        treePSDesc.entryPoint = "main";
        treePSDesc.debugName = "TreePS";

        g_app.treeVertexShader = g_app.device->CreateShader(treeVSDesc);
        g_app.treePixelShader = g_app.device->CreateShader(treePSDesc);

        if (g_app.treeVertexShader && g_app.treePixelShader) {
            // Create tree pipeline
            PipelineDesc treePipeDesc;
            treePipeDesc.vertexShader = g_app.treeVertexShader.get();
            treePipeDesc.pixelShader = g_app.treePixelShader.get();

            // Vertex layout: Position (3) + pad + Normal (3) + pad + TexCoord (2) = 40 bytes
            VertexAttribute posAttr;
            posAttr.semanticName = "POSITION";
            posAttr.format = Format::R32G32B32_FLOAT;
            posAttr.offset = 0;
            posAttr.inputSlot = 0;
            posAttr.instanceStepRate = 0;
            treePipeDesc.vertexLayout.push_back(posAttr);

            VertexAttribute normalAttr;
            normalAttr.semanticName = "NORMAL";
            normalAttr.format = Format::R32G32B32_FLOAT;
            normalAttr.offset = 16;  // 12 + 4 padding
            normalAttr.inputSlot = 0;
            normalAttr.instanceStepRate = 0;
            treePipeDesc.vertexLayout.push_back(normalAttr);

            VertexAttribute texCoordAttr;
            texCoordAttr.semanticName = "TEXCOORD";
            texCoordAttr.format = Format::R32G32_FLOAT;
            texCoordAttr.offset = 32;  // 16 + 12 + 4 padding
            texCoordAttr.inputSlot = 0;
            texCoordAttr.instanceStepRate = 0;
            treePipeDesc.vertexLayout.push_back(texCoordAttr);

            treePipeDesc.primitiveTopology = PrimitiveTopology::TriangleList;
            treePipeDesc.cullMode = CullMode::Back;
            treePipeDesc.depthTestEnabled = true;
            treePipeDesc.depthWriteEnabled = true;
            treePipeDesc.depthCompareOp = CompareOp::Less;
            treePipeDesc.blendEnabled = false;
            treePipeDesc.renderTargetFormats.push_back(Format::R8G8B8A8_UNORM);
            treePipeDesc.depthStencilFormat = Format::D32_FLOAT;
            treePipeDesc.debugName = "TreePipeline";

            g_app.treePipeline = g_app.device->CreatePipeline(treePipeDesc);

            if (g_app.treePipeline) {
                // Create tree renderer
                g_app.treeRenderer = std::make_unique<TreeRendererDX12>();
                if (g_app.treeRenderer->init(g_app.device.get(), g_app.vegetationManager.get())) {
                    // Generate tree meshes
                    g_app.treeRenderer->generateTreeMeshes();

                    std::cout << "  Tree rendering initialized successfully" << std::endl;
                    std::cout << "  Total trees: " << g_app.vegetationManager->getTreeInstances().size() << std::endl;
                    g_app.treeRenderingEnabled = true;
                } else {
                    std::cerr << "  WARNING: Tree renderer init failed!" << std::endl;
                    g_app.treeRenderingEnabled = false;
                }
            } else {
                std::cerr << "  WARNING: Failed to create tree pipeline!" << std::endl;
                g_app.treeRenderingEnabled = false;
            }
        } else {
            std::cerr << "  WARNING: Failed to compile tree shaders!" << std::endl;
            g_app.treeRenderingEnabled = false;
        }
    }

    // Initialize terrain rendering system (Phase 5 - 3D World)
    std::cout << "Initializing terrain rendering..." << std::endl;
    {
        // Terrain vertex shader source (embedded) - Enhanced with water animation
        static const char* terrainVSSource = R"HLSL(
// Terrain Vertex Shader - Enhanced with water wave animation
cbuffer TerrainConstants : register(b0) {
    float4x4 viewProj;
    float4x4 world;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 terrainScale;    // x=heightScale, y=chunkSize, z=waterLevel, w=time
    float4 texCoordScale;   // x=u scale, y=v scale
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float2 texCoord : TEXCOORD3;
    float height : TEXCOORD4;
};

PSInput main(VSInput input) {
    PSInput output;

    float3 pos = input.position;
    float heightScale = terrainScale.x;
    float waterLevel = terrainScale.z;
    float time = terrainScale.w;

    // Calculate normalized height
    float normalizedHeight = pos.y / heightScale;

    // Animate water vertices with waves
    if (normalizedHeight < waterLevel + 0.02) {
        float waveSpeed = 0.4;
        float waveHeight = 0.15;  // Wave amplitude
        float waveFreq = 1.5;

        // How much to animate based on depth below water
        float waterFactor = saturate((waterLevel - normalizedHeight) / 0.15);

        // Multiple wave layers for realistic motion
        pos.y += sin(pos.x * waveFreq + time * waveSpeed) * waveHeight * waterFactor;
        pos.y += cos(pos.z * waveFreq * 1.3 + time * waveSpeed * 0.8) * waveHeight * 0.6 * waterFactor;
        pos.y += sin((pos.x + pos.z) * waveFreq * 0.7 + time * waveSpeed * 1.2) * waveHeight * 0.3 * waterFactor;
    }

    float4 worldPos = mul(world, float4(pos, 1.0));
    output.position = mul(viewProj, worldPos);
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)world, input.normal));
    output.color = input.color;
    output.texCoord = input.texCoord * texCoordScale.xy;
    output.height = input.position.y / heightScale;  // Use original height for biome calculation
    return output;
}
)HLSL";

        // Terrain pixel shader source (embedded) - Enhanced with water, mountains, and biomes
        static const char* terrainPSSource = R"HLSL(
// Terrain Pixel Shader - Enhanced with realistic water, mountains, and biome effects
cbuffer TerrainConstants : register(b0) {
    float4x4 viewProj;
    float4x4 world;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 terrainScale;    // x=heightScale, y=chunkSize, z=waterLevel, w=time
    float4 texCoordScale;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float2 texCoord : TEXCOORD3;
    float height : TEXCOORD4;
};

// Noise functions for procedural effects
float3 hash3(float3 p) {
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float noise3D(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(lerp(hash3(i).x, hash3(i + float3(1,0,0)).x, f.x),
             lerp(hash3(i + float3(0,1,0)).x, hash3(i + float3(1,1,0)).x, f.x), f.y),
        lerp(lerp(hash3(i + float3(0,0,1)).x, hash3(i + float3(1,0,1)).x, f.x),
             lerp(hash3(i + float3(0,1,1)).x, hash3(i + float3(1,1,1)).x, f.x), f.y),
        f.z
    );
}

float fbm(float3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        if (i >= octaves) break;
        value += amplitude * noise3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Voronoi for caustics and rock patterns
float voronoi(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    float minDist = 1.0;
    for (int z = -1; z <= 1; z++) {
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                float3 neighbor = float3(x, y, z);
                float3 cellPoint = hash3(i + neighbor);
                float3 diff = neighbor + cellPoint - f;
                minDist = min(minDist, length(diff));
            }
        }
    }
    return minDist;
}

// Biome thresholds (normalized height 0-1)
#define WATER_LEVEL terrainScale.z
#define BEACH_LEVEL (terrainScale.z + 0.07)
static const float GRASS_LEVEL = 0.65;
static const float FOREST_LEVEL = 0.80;
static const float ROCK_LEVEL = 0.92;

// Biome colors
static const float3 DEEP_WATER = float3(0.02, 0.12, 0.25);
static const float3 SHALLOW_WATER = float3(0.1, 0.35, 0.55);
static const float3 BEACH_COLOR = float3(0.85, 0.78, 0.55);
static const float3 GRASS_COLOR = float3(0.25, 0.55, 0.2);
static const float3 FOREST_COLOR = float3(0.15, 0.4, 0.15);
static const float3 ROCK_COLOR = float3(0.45, 0.42, 0.4);
static const float3 SNOW_COLOR = float3(0.95, 0.95, 1.0);

// Calculate animated water normal
float3 calculateWaterNormal(float3 worldPos, float time) {
    float2 uv = worldPos.xz;

    // Multiple wave layers
    float2 wave1UV = uv * 0.06 + float2(time * 0.015, time * 0.01);
    float wave1 = fbm(float3(wave1UV, 0.0), 3);

    float2 wave2UV = uv * 0.12 + float2(-time * 0.02, time * 0.025);
    float wave2 = fbm(float3(wave2UV, 1.0), 3);

    float2 wave3UV = uv * 0.3 + float2(time * 0.04, -time * 0.03);
    float wave3 = noise3D(float3(wave3UV, 2.0));

    float combinedWave = wave1 * 0.5 + wave2 * 0.35 + wave3 * 0.15;

    // Calculate normal from derivatives
    float eps = 0.15;
    float2 wave1dx = (uv + float2(eps, 0)) * 0.06 + float2(time * 0.015, time * 0.01);
    float2 wave1dz = (uv + float2(0, eps)) * 0.06 + float2(time * 0.015, time * 0.01);

    float heightX = fbm(float3(wave1dx, 0.0), 3) * 0.5;
    float heightZ = fbm(float3(wave1dz, 0.0), 3) * 0.5;

    float dx = (heightX - combinedWave) / eps;
    float dz = (heightZ - combinedWave) / eps;

    return normalize(float3(-dx * 0.4, 1.0, -dz * 0.4));
}

// Fresnel effect for water reflections
float fresnel(float3 viewDir, float3 normal, float power) {
    return pow(1.0 - saturate(dot(viewDir, normal)), power);
}

// Generate realistic water color
float3 generateWaterColor(float3 worldPos, float3 viewDir, float3 lightDir, float normalizedHeight, float time) {
    float3 waterNormal = calculateWaterNormal(worldPos, time);

    // Water depth
    float depth = saturate((WATER_LEVEL - normalizedHeight) / 0.25);

    // Fresnel effect
    float fres = fresnel(viewDir, waterNormal, 3.0);
    fres += noise3D(float3(worldPos.xz * 0.2 + time * 0.05, 0.0)) * 0.1;
    fres = saturate(fres);

    // Refraction (underwater) color
    float3 refractionColor = lerp(SHALLOW_WATER, DEEP_WATER, depth);

    // Add caustics in shallow water
    float2 causticUV = worldPos.xz * 0.25;
    float caustic = voronoi(float3(causticUV + time * 0.04, 0.0));
    caustic = pow(caustic, 0.5) * 0.25 * (1.0 - depth);
    refractionColor += float3(caustic, caustic * 0.9, caustic * 0.7);

    // Reflection color (sky)
    float3 reflectedView = reflect(-viewDir, waterNormal);
    float skyGradient = saturate(reflectedView.y * 0.5 + 0.5);
    float3 skyColor = lerp(float3(0.6, 0.75, 0.85), float3(0.4, 0.6, 0.9), skyGradient);

    // Blend refraction and reflection
    float3 waterColor = lerp(refractionColor, skyColor, fres);

    // Sun specular
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(waterNormal, halfVec));
    float specular = pow(NdotH, 256.0) + pow(NdotH, 64.0) * 0.3;
    waterColor += float3(1.0, 0.95, 0.8) * specular;

    // Foam at shoreline
    float shorelineFoam = 1.0 - smoothstep(0.0, 0.12, depth);
    float foamNoise = noise3D(float3(worldPos.xz * 0.4 + time * 0.08, time * 0.15));
    float foam = shorelineFoam * foamNoise;
    waterColor = lerp(waterColor, float3(0.9, 0.95, 1.0), foam * 0.7);

    return waterColor;
}

// Generate sand color with variation
float3 generateSandColor(float3 pos) {
    float sandNoise = fbm(pos * 12.0, 3);
    return lerp(float3(0.75, 0.68, 0.48), BEACH_COLOR, sandNoise);
}

// Generate grass color with variation
float3 generateGrassColor(float3 pos, float3 normal) {
    float grassNoise = fbm(pos * 6.0, 3);
    float detail = noise3D(pos * 30.0) * 0.15;
    float3 lightGrass = float3(0.35, 0.6, 0.25);
    float3 darkGrass = float3(0.18, 0.42, 0.15);
    return lerp(darkGrass, lightGrass, grassNoise) * (1.0 + detail);
}

// Generate rock color with variation
float3 generateRockColor(float3 pos, float3 normal) {
    float rockPattern = voronoi(pos * 4.0);
    float rockDetail = fbm(pos * 8.0, 4);
    float3 darkRock = float3(0.35, 0.32, 0.3);
    float3 lightRock = float3(0.55, 0.52, 0.5);
    return lerp(darkRock, lightRock, rockPattern * 0.6 + rockDetail * 0.4);
}

// Generate snow color with sparkle
float3 generateSnowColor(float3 pos, float time) {
    float snowNoise = noise3D(pos * 15.0) * 0.1;
    float sparkle = pow(noise3D(pos * 50.0 + time * 2.0), 8.0) * 0.3;
    return SNOW_COLOR * (1.0 - snowNoise) + float3(sparkle, sparkle, sparkle);
}

float4 main(PSInput input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir.xyz);
    float3 V = normalize(cameraPos.xyz - input.worldPos);
    float time = terrainScale.w;

    // Get normalized height (0-1 range)
    float normalizedHeight = input.height;

    // Calculate slope for rock blending on steep surfaces
    float slope = 1.0 - N.y;
    float rockBlend = smoothstep(0.4, 0.7, slope);

    // Generate biome color based on height with smooth transitions
    float3 terrainColor;
    float blendRange = 0.04;

    if (normalizedHeight < WATER_LEVEL) {
        // Water
        terrainColor = generateWaterColor(input.worldPos, V, L, normalizedHeight, time);
    }
    else if (normalizedHeight < BEACH_LEVEL) {
        // Beach with water transition
        float t = smoothstep(WATER_LEVEL - blendRange, WATER_LEVEL + blendRange, normalizedHeight);
        float3 waterCol = generateWaterColor(input.worldPos, V, L, normalizedHeight, time);
        float3 sandCol = generateSandColor(input.worldPos);
        terrainColor = lerp(waterCol, sandCol, t);
    }
    else if (normalizedHeight < GRASS_LEVEL) {
        // Grass with beach/forest transitions
        float t1 = smoothstep(BEACH_LEVEL - blendRange, BEACH_LEVEL + blendRange, normalizedHeight);
        float t2 = smoothstep(GRASS_LEVEL - blendRange * 2.0, GRASS_LEVEL, normalizedHeight);
        float3 sandCol = generateSandColor(input.worldPos);
        float3 grassCol = generateGrassColor(input.worldPos, N);
        float3 forestCol = lerp(GRASS_COLOR, FOREST_COLOR, 0.5);

        if (normalizedHeight < BEACH_LEVEL + blendRange) {
            terrainColor = lerp(sandCol, grassCol, t1);
        } else {
            terrainColor = lerp(grassCol, forestCol, t2);
        }
    }
    else if (normalizedHeight < ROCK_LEVEL) {
        // Forest to rock transition
        float t = smoothstep(FOREST_LEVEL - blendRange, ROCK_LEVEL, normalizedHeight);
        float3 forestCol = FOREST_COLOR;
        float3 rockCol = generateRockColor(input.worldPos, N);
        terrainColor = lerp(forestCol, rockCol, t);
    }
    else {
        // Rock to snow transition (mountain peaks)
        float t = smoothstep(ROCK_LEVEL - blendRange, 1.0, normalizedHeight);
        float3 rockCol = generateRockColor(input.worldPos, N);
        float3 snowCol = generateSnowColor(input.worldPos, time);
        terrainColor = lerp(rockCol, snowCol, t);
    }

    // Blend rock on steep slopes (except water)
    if (normalizedHeight >= WATER_LEVEL && normalizedHeight < ROCK_LEVEL) {
        float3 rockCol = generateRockColor(input.worldPos, N);
        terrainColor = lerp(terrainColor, rockCol, rockBlend);
    }

    // Lighting (skip for water which has its own lighting)
    float3 finalColor;
    if (normalizedHeight < WATER_LEVEL) {
        finalColor = terrainColor;
    } else {
        float NdotL = max(dot(N, L), 0.0);

        // Ambient with hemisphere lighting
        float3 skyAmbient = float3(0.5, 0.6, 0.8) * 0.25;
        float3 groundAmbient = float3(0.3, 0.25, 0.2) * 0.15;
        float3 ambient = lerp(groundAmbient, skyAmbient, N.y * 0.5 + 0.5);

        // Diffuse
        float3 diffuse = terrainColor * lightColor.rgb * NdotL * 0.7;

        // Subtle specular for snow
        float specular = 0.0;
        if (normalizedHeight > ROCK_LEVEL) {
            float3 H = normalize(L + V);
            specular = pow(max(dot(N, H), 0.0), 32.0) * 0.3;
        }

        finalColor = ambient * terrainColor + diffuse + float3(specular, specular, specular);
    }

    // Atmospheric fog with proper sky rendering
    float dist = length(cameraPos.xyz - input.worldPos);
    float fogFactor = 1.0 - exp(-dist * 0.003);
    fogFactor = saturate(fogFactor);

    // Calculate view direction for sky rendering
    float3 fogViewDir = normalize(input.worldPos - cameraPos.xyz);

    // Sky gradient: darker blue at zenith, lighter at horizon
    float horizonFactor = 1.0 - saturate(fogViewDir.y);  // 0 at zenith, 1 at horizon
    float zenithFactor = saturate(fogViewDir.y);

    // Sky colors
    float3 zenithColor = float3(0.25, 0.45, 0.85);    // Darker blue at top
    float3 horizonColor = float3(0.7, 0.82, 0.95);    // Light blue/white at horizon
    float3 belowHorizonColor = float3(0.5, 0.6, 0.75); // Slightly darker below horizon

    // Create sky gradient
    float3 skyColor;
    if (fogViewDir.y >= 0.0) {
        // Above horizon: smooth gradient from horizon to zenith
        float t = pow(zenithFactor, 0.5);  // Non-linear for more realistic falloff
        skyColor = lerp(horizonColor, zenithColor, t);
    } else {
        // Below horizon: fade to slightly darker
        float t = saturate(-fogViewDir.y * 2.0);
        skyColor = lerp(horizonColor, belowHorizonColor, t);
    }

    // Sun disc rendering
    float3 sunDir = normalize(-lightDir.xyz);
    float sunDot = dot(fogViewDir, sunDir);

    // Sun disc (sharp edge)
    float sunRadius = 0.995;  // Cosine of sun angular radius (~5.7 degrees)
    float sunDisc = smoothstep(sunRadius - 0.002, sunRadius + 0.001, sunDot);

    // Sun glow (soft halo around sun)
    float sunGlow = pow(saturate(sunDot), 8.0) * 0.4;
    float sunGlowIntense = pow(saturate(sunDot), 64.0) * 0.3;

    // Sun colors
    float3 sunColor = float3(1.0, 0.98, 0.9);      // Bright white-yellow
    float3 sunGlowColor = float3(1.0, 0.9, 0.6);  // Warm yellow glow

    // Add sun to sky
    skyColor += sunGlowColor * sunGlow;
    skyColor += sunGlowColor * sunGlowIntense;
    skyColor = lerp(skyColor, sunColor, sunDisc);

    // Procedural clouds using existing noise functions
    if (fogViewDir.y > 0.0) {
        // Cloud layer - project view direction onto a virtual cloud plane
        float cloudHeight = 500.0;  // Virtual height of cloud layer
        float3 cloudSamplePos = cameraPos.xyz + fogViewDir * (cloudHeight / max(fogViewDir.y, 0.01));

        // Cloud noise at multiple frequencies
        float2 cloudUV = cloudSamplePos.xz * 0.0008;  // Scale for cloud size
        float cloudNoise1 = fbm(float3(cloudUV + time * 0.002, 0.0), 4);
        float cloudNoise2 = fbm(float3(cloudUV * 2.0 + time * 0.003, 1.0), 3);
        float cloudNoise3 = noise3D(float3(cloudUV * 4.0 + time * 0.005, 2.0));

        // Combine noise layers for cloud shape
        float cloudDensity = cloudNoise1 * 0.6 + cloudNoise2 * 0.3 + cloudNoise3 * 0.1;

        // Create cloud coverage with threshold
        float cloudCoverage = 0.45;  // How much of sky is covered
        float cloudSharpness = 3.0;
        float cloud = saturate((cloudDensity - (1.0 - cloudCoverage)) * cloudSharpness);

        // Cloud edges are more transparent near horizon
        float cloudFade = smoothstep(0.0, 0.15, fogViewDir.y);
        cloud *= cloudFade;

        // Cloud colors - brighter on sun side
        float cloudSunLight = saturate(dot(float3(0, 1, 0), sunDir) * 0.5 + 0.5);
        float3 cloudBright = float3(1.0, 1.0, 1.0);
        float3 cloudDark = float3(0.75, 0.78, 0.85);
        float3 cloudColor = lerp(cloudDark, cloudBright, cloudSunLight * 0.5 + 0.5);

        // Add slight orange tint when looking toward sun
        float sunSideCloud = pow(saturate(sunDot * 0.5 + 0.5), 2.0);
        cloudColor = lerp(cloudColor, float3(1.0, 0.95, 0.85), sunSideCloud * 0.2);

        // Blend clouds with sky
        skyColor = lerp(skyColor, cloudColor, cloud * 0.85);
    }

    // Final fog color is the rendered sky
    float3 fogColor = skyColor;

    finalColor = lerp(finalColor, fogColor, fogFactor);

    return float4(finalColor, 1.0);
}
)HLSL";

        // Compile terrain shaders
        ShaderDesc terrainVSDesc;
        terrainVSDesc.type = ShaderType::Vertex;
        terrainVSDesc.source = terrainVSSource;
        terrainVSDesc.entryPoint = "main";
        terrainVSDesc.debugName = "TerrainVS";

        ShaderDesc terrainPSDesc;
        terrainPSDesc.type = ShaderType::Pixel;
        terrainPSDesc.source = terrainPSSource;
        terrainPSDesc.entryPoint = "main";
        terrainPSDesc.debugName = "TerrainPS";

        g_app.terrainVertexShader = g_app.device->CreateShader(terrainVSDesc);
        g_app.terrainPixelShader = g_app.device->CreateShader(terrainPSDesc);

        if (g_app.terrainVertexShader && g_app.terrainPixelShader) {
            // Create terrain pipeline
            PipelineDesc terrainPipeDesc;
            terrainPipeDesc.vertexShader = g_app.terrainVertexShader.get();
            terrainPipeDesc.pixelShader = g_app.terrainPixelShader.get();

            // Vertex layout: Position (3 floats) + Normal (3 floats) + Color (3 floats) + TexCoord (2 floats) = 44 bytes
            VertexAttribute posAttr;
            posAttr.semanticName = "POSITION";
            posAttr.format = Format::R32G32B32_FLOAT;
            posAttr.offset = 0;
            posAttr.inputSlot = 0;
            posAttr.instanceStepRate = 0;
            terrainPipeDesc.vertexLayout.push_back(posAttr);

            VertexAttribute normalAttr;
            normalAttr.semanticName = "NORMAL";
            normalAttr.format = Format::R32G32B32_FLOAT;
            normalAttr.offset = 12;
            normalAttr.inputSlot = 0;
            normalAttr.instanceStepRate = 0;
            terrainPipeDesc.vertexLayout.push_back(normalAttr);

            VertexAttribute colorAttr;
            colorAttr.semanticName = "COLOR";
            colorAttr.format = Format::R32G32B32_FLOAT;
            colorAttr.offset = 24;
            colorAttr.inputSlot = 0;
            colorAttr.instanceStepRate = 0;
            terrainPipeDesc.vertexLayout.push_back(colorAttr);

            VertexAttribute texCoordAttr;
            texCoordAttr.semanticName = "TEXCOORD";
            texCoordAttr.format = Format::R32G32_FLOAT;
            texCoordAttr.offset = 36;
            texCoordAttr.inputSlot = 0;
            texCoordAttr.instanceStepRate = 0;
            terrainPipeDesc.vertexLayout.push_back(texCoordAttr);

            terrainPipeDesc.primitiveTopology = PrimitiveTopology::TriangleList;
            terrainPipeDesc.cullMode = CullMode::Back;
            terrainPipeDesc.depthTestEnabled = true;
            terrainPipeDesc.depthWriteEnabled = true;
            terrainPipeDesc.depthCompareOp = CompareOp::Less;
            terrainPipeDesc.blendEnabled = false;
            terrainPipeDesc.renderTargetFormats.push_back(Format::R8G8B8A8_UNORM);
            terrainPipeDesc.depthStencilFormat = Format::D32_FLOAT;
            terrainPipeDesc.debugName = "TerrainPipeline";

            g_app.terrainPipeline = g_app.device->CreatePipeline(terrainPipeDesc);

            if (g_app.terrainPipeline) {
                // Create terrain renderer using self-contained procedural generation
                g_app.terrainRenderer = std::make_unique<TerrainRendererDX12>();
                if (g_app.terrainRenderer->init(g_app.device.get())) {
                    std::cout << "  Terrain rendering initialized successfully" << std::endl;
                    std::cout << "  Chunks: " << g_app.terrainRenderer->getRenderedChunkCount() << std::endl;
                    std::cout << "  Vertices: " << g_app.terrainRenderer->getTotalVertices() << std::endl;
                    std::cout << "  Indices: " << g_app.terrainRenderer->getTotalIndices() << std::endl;
                    g_app.terrainRenderingEnabled = true;
                } else {
                    std::cerr << "  WARNING: Terrain renderer init failed!" << std::endl;
                    g_app.terrainRenderingEnabled = false;
                }
            } else {
                std::cerr << "  WARNING: Failed to create terrain pipeline!" << std::endl;
                g_app.terrainRenderingEnabled = false;
            }
        } else {
            std::cerr << "  WARNING: Failed to compile terrain shaders!" << std::endl;
            g_app.terrainRenderingEnabled = false;
        }
    }

    // Initialize simulation with creature pool
    std::cout << "Initializing simulation with creature pool..." << std::endl;
    std::cout << "  Pool mode: " << (g_app.world.usePooling ? "ENABLED" : "DISABLED") << std::endl;
    g_app.world.Initialize(1000, 100, 500);  // 1000 herbivores, 100 carnivores, 500 food
    std::cout << "  Pool capacity: " << g_app.world.creaturePool.getTotalCapacity() << " creatures" << std::endl;
    std::cout << "  Active creatures: " << g_app.world.creaturePool.getActiveCount() << std::endl;
    std::cout << "  Pool utilization: " << std::fixed << std::setprecision(1)
              << (g_app.world.creaturePool.getUtilization() * 100.0f) << "%" << std::endl;

    // Initialize save system with auto-save
    std::cout << "Initializing save system..." << std::endl;
    g_app.saveManager.ensureSaveDirectory();
    g_app.saveManager.enableAutoSave(300.0f);  // Auto-save every 5 minutes
    g_app.saveManager.setAutoSaveCallback([](const std::string& path) {
        SaveGame(path);
        g_app.statusMessage = "Auto-saved";
        g_app.statusMessageTimer = 2.0f;
    });
    std::cout << "  Save directory: " << g_app.saveManager.getSaveDirectory() << std::endl;
    std::cout << "  Auto-save enabled (every 5 minutes)" << std::endl;

    // Initialize replay recording (S-05)
    std::cout << "Initializing replay system..." << std::endl;
    g_app.replayRecorder.setRecordInterval(1.0f);  // 1 frame per second
    g_app.replayRecorder.setMaxFrames(36000);       // 10 hours max recording
    g_app.replayRecorder.startRecording(42);        // Use fixed seed for reproducibility
    g_app.isRecording = true;
    std::cout << "  Recording started (1 fps, max 10 hours)" << std::endl;

    // Initialize gameplay systems (Agent 26 - Fun Factor)
    std::cout << "Initializing gameplay systems..." << std::endl;
    g_app.gameplay.initialize();
    g_app.gameplay.addEvent("Welcome to Organism Evolution!", Forge::GameEventType::INFO, true);
    std::cout << "  Gameplay manager initialized" << std::endl;
    std::cout << "  Achievements, events, and highlights ready" << std::endl;

    // PHASE 10 - Agent 1: Initialize CreatureManager (unified creature system)
    std::cout << "Initializing CreatureManager (Phase 10 - unified simulation)..." << std::endl;
    g_app.creatureManager = std::make_unique<Forge::CreatureManager>(500.0f, 500.0f);
    // Note: terrain and ecosystem managers are not yet created in this codebase
    // For now, initialize with nullptr - spawn will still work
    g_app.creatureManager->init(nullptr, nullptr, 42);
    std::cout << "  CreatureManager initialized (max: " << Forge::CreatureManager::MAX_CREATURES << " creatures)" << std::endl;
    std::cout << "  Spatial grid resolution: " << Forge::CreatureManager::GRID_RESOLUTION << "x" << Forge::CreatureManager::GRID_RESOLUTION << std::endl;

    std::cout << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Simulation started!" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left Mouse  - Capture/release mouse (FPS look)" << std::endl;
    std::cout << "  WASD        - Move camera (FPS) / Move target (orbit)" << std::endl;
    std::cout << "  Right Mouse - Rotate camera (orbit)" << std::endl;
    std::cout << "  R           - Reset camera to default position" << std::endl;
    std::cout << "  Space/P     - Toggle pause" << std::endl;
    std::cout << "  1-6         - Set simulation speed (0.25x to 8x)" << std::endl;
    std::cout << "  F1          - Toggle debug panel" << std::endl;
    std::cout << "  F2          - Toggle performance profiler" << std::endl;
    std::cout << "  F3          - Toggle help overlay" << std::endl;
    std::cout << "  F5          - Quick save" << std::endl;
    std::cout << "  F9          - Quick load" << std::endl;
    std::cout << "  F10         - Toggle replay mode" << std::endl;
    std::cout << "  ESC         - Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Gameplay Features:" << std::endl;
    std::cout << "  G           - Toggle live statistics panel" << std::endl;
    std::cout << "  H           - Toggle creature highlighting" << std::endl;
    std::cout << "  J           - Toggle achievements panel" << std::endl;
    std::cout << "  K           - Cycle spotlight creature" << std::endl;
    std::cout << "  Shift+1-7   - Quick highlight mode selection" << std::endl;
    std::cout << std::endl;

    // Initialize timing
    g_app.lastFrameTime = std::chrono::high_resolution_clock::now();

    // Main loop
    while (g_app.window->PollEvents()) {
        // Check for escape key
        if (g_app.window->IsKeyDown(KeyCode::Escape) && !g_app.mouseCaptured) {
            break;
        }

        // Skip rendering if minimized
        if (g_app.window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // Handle input
        HandleInput();

        // Update
        UpdateFrame();

        // Render
        if (g_app.worldDiagnostics && g_app.worldDiagnosticsFrames > 0) {
            AppendRuntimeDiagLog("Main loop before RenderFrame.");
        }
        RenderFrame();
    }

    // Cleanup
    std::cout << std::endl;
    std::cout << "Shutting down..." << std::endl;
    Cleanup();

    std::cout << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Simulation ended. Thank you for using OrganismEvolution!" << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
