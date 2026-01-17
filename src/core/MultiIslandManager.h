#pragma once

// MultiIslandManager - Manages multiple islands in an archipelago
// Each island has its own Terrain, CreatureManager, and VegetationManager

#include "../environment/ArchipelagoGenerator.h"
#include "../environment/Terrain.h"
#include "../environment/VegetationManager.h"
#include "CreatureManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>

class Camera;
class DX12Device;
class IslandGenerator;
class ClimateSystem;
struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12GraphicsCommandList;

namespace Forge {

// Forward declarations
class InterIslandMigration;

// ============================================================================
// Island Statistics
// ============================================================================

struct IslandStats {
    int totalCreatures = 0;
    int speciesCount = 0;
    float avgFitness = 0.0f;
    float avgEnergy = 0.0f;
    float geneticDiversity = 0.0f;
    int births = 0;
    int deaths = 0;
    int immigrations = 0;
    int emigrations = 0;
    float vegetationDensity = 0.0f;

    void reset() {
        totalCreatures = speciesCount = births = deaths = immigrations = emigrations = 0;
        avgFitness = avgEnergy = geneticDiversity = vegetationDensity = 0.0f;
    }
};

// ============================================================================
// Island Structure
// ============================================================================

struct Island {
    uint32_t id;
    std::string name;
    glm::vec2 worldPosition;        // 2D position in archipelago space
    float size;                     // Size multiplier

    // Core systems (each island has its own)
    std::unique_ptr<Terrain> terrain;
    std::unique_ptr<CreatureManager> creatures;
    std::unique_ptr<VegetationManager> vegetation;

    // Optional systems
    ClimateSystem* climate = nullptr;

    // Configuration
    IslandConfig config;

    // Statistics
    IslandStats stats;

    // State flags
    bool isLoaded = false;
    bool isActive = false;
    bool needsUpdate = true;

    // Transform from local to world coordinates
    glm::vec3 localToWorld(const glm::vec3& localPos) const {
        return glm::vec3(
            localPos.x + worldPosition.x,
            localPos.y,
            localPos.z + worldPosition.y
        );
    }

    // Transform from world to local coordinates
    glm::vec3 worldToLocal(const glm::vec3& worldPos) const {
        return glm::vec3(
            worldPos.x - worldPosition.x,
            worldPos.y,
            worldPos.z - worldPosition.y
        );
    }

    // Get terrain bounds in world space
    void getWorldBounds(glm::vec3& outMin, glm::vec3& outMax) const {
        if (!terrain) return;

        float halfWidth = terrain->getWidth() * terrain->getScale() * 0.5f;
        float halfDepth = terrain->getDepth() * terrain->getScale() * 0.5f;

        outMin = glm::vec3(
            worldPosition.x - halfWidth,
            0.0f,
            worldPosition.y - halfDepth
        );

        outMax = glm::vec3(
            worldPosition.x + halfWidth,
            100.0f,  // Approximate max height
            worldPosition.y + halfDepth
        );
    }
};

// ============================================================================
// Island Event
// ============================================================================

struct IslandEvent {
    enum class Type {
        ISLAND_ACTIVATED,
        ISLAND_DEACTIVATED,
        CREATURE_MIGRATED_OUT,
        CREATURE_MIGRATED_IN,
        SPECIES_EXTINCT,
        SPECIES_FORMED,
        POPULATION_BOOM,
        POPULATION_CRASH
    };

    Type type;
    uint32_t islandId;
    uint32_t creatureId = 0;
    uint32_t speciesId = 0;
    std::string description;
    float timestamp = 0.0f;
};

// ============================================================================
// Multi-Island Manager
// ============================================================================

class MultiIslandManager {
public:
    // Configuration
    static constexpr int MAX_ISLANDS = 16;
    static constexpr int DEFAULT_TERRAIN_SIZE = 256;   // Size per island (smaller for performance)
    static constexpr float DEFAULT_TERRAIN_SCALE = 1.0f;

    // Callback types
    using EventCallback = std::function<void(const IslandEvent&)>;

    MultiIslandManager();
    ~MultiIslandManager();

    // ========================================================================
    // Initialization
    // ========================================================================

    // Initialize from archipelago generator
    void init(const ArchipelagoGenerator& archipelago);

    // Initialize DX12 resources for all islands
    void initializeDX12(DX12Device* device, ID3D12PipelineState* terrainPSO,
                        ID3D12RootSignature* rootSig);

    // Generate all island terrain and populate with creatures
    void generateAll(unsigned int baseSeed);

    // Clear all islands
    void clear();

    // ========================================================================
    // Island Access
    // ========================================================================

    Island* getIsland(uint32_t index);
    const Island* getIsland(uint32_t index) const;

    Island* getActiveIsland();
    const Island* getActiveIsland() const;

    void setActiveIsland(uint32_t index);
    uint32_t getActiveIslandIndex() const { return m_activeIslandIndex; }

    uint32_t getIslandCount() const { return static_cast<uint32_t>(m_islands.size()); }

    // Find island containing world position
    int findIslandAt(const glm::vec3& worldPos) const;

    // ========================================================================
    // Update
    // ========================================================================

    // Update all active islands
    void update(float deltaTime);

    // Update specific island (for LOD/streaming)
    void updateIsland(uint32_t index, float deltaTime);

    // Update statistics for all islands
    void updateStatistics();

    // ========================================================================
    // Rendering
    // ========================================================================

    // Render all visible islands
    void render(const Camera& camera, ID3D12GraphicsCommandList* commandList);

    // Render specific island
    void renderIsland(uint32_t index, ID3D12GraphicsCommandList* commandList);

    // Render for shadow pass
    void renderForShadow(const Camera& camera, ID3D12GraphicsCommandList* commandList);

    // ========================================================================
    // Creature Management
    // ========================================================================

    // Get total creature count across all islands
    int getTotalCreatureCount() const;

    // Get creature count for specific island
    int getCreatureCount(uint32_t islandIndex) const;

    // Spawn creature on specific island
    CreatureHandle spawnCreature(uint32_t islandIndex, CreatureType type,
                                  const glm::vec3& localPosition,
                                  const Genome* parentGenome = nullptr);

    // Transfer creature between islands (for migration)
    bool transferCreature(uint32_t fromIsland, uint32_t toIsland,
                         CreatureHandle handle, const glm::vec3& arrivalPosition);

    // ========================================================================
    // Inter-Island Queries
    // ========================================================================

    // Get distance between two islands
    float getIslandDistance(uint32_t islandA, uint32_t islandB) const;

    // Get neighboring islands within range
    std::vector<uint32_t> getNeighborIslands(uint32_t islandIndex, float maxDistance) const;

    // Check if position is on any island
    bool isOnAnyIsland(const glm::vec3& worldPos) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const IslandStats& getIslandStats(uint32_t index) const;
    IslandStats getGlobalStats() const;

    // Get genetic distance between islands
    float getGeneticDistance(uint32_t islandA, uint32_t islandB) const;

    // ========================================================================
    // Events
    // ========================================================================

    void registerEventCallback(EventCallback callback);
    const std::vector<IslandEvent>& getRecentEvents() const { return m_recentEvents; }
    void clearEvents() { m_recentEvents.clear(); }

    // ========================================================================
    // Configuration
    // ========================================================================

    void setTerrainSize(int size) { m_terrainSize = size; }
    void setTerrainScale(float scale) { m_terrainScale = scale; }
    void setMaxCreaturesPerIsland(size_t max) { m_maxCreaturesPerIsland = max; }

    // LOD settings
    void setUpdateRadiusForInactiveIslands(float radius) { m_inactiveUpdateRadius = radius; }
    void setAlwaysUpdateActiveIsland(bool always) { m_alwaysUpdateActive = always; }

    // ========================================================================
    // Archipelago Data Access
    // ========================================================================

    const ArchipelagoData& getArchipelagoData() const { return m_archipelagoData; }
    const std::vector<OceanCurrent>& getOceanCurrents() const { return m_archipelagoData.currents; }

private:
    // Islands
    std::vector<Island> m_islands;
    uint32_t m_activeIslandIndex = 0;

    // Archipelago configuration
    ArchipelagoData m_archipelagoData;

    // DX12 resources
    DX12Device* m_dx12Device = nullptr;
    ID3D12PipelineState* m_terrainPSO = nullptr;
    ID3D12RootSignature* m_rootSignature = nullptr;

    // Configuration
    int m_terrainSize = DEFAULT_TERRAIN_SIZE;
    float m_terrainScale = DEFAULT_TERRAIN_SCALE;
    size_t m_maxCreaturesPerIsland = 2048;

    // LOD/Update settings
    float m_inactiveUpdateRadius = 500.0f;  // Distance from camera to update inactive islands
    bool m_alwaysUpdateActive = true;

    // Events
    std::vector<EventCallback> m_eventCallbacks;
    std::vector<IslandEvent> m_recentEvents;
    static constexpr size_t MAX_RECENT_EVENTS = 100;

    // Statistics cache
    mutable IslandStats m_globalStatsCache;
    mutable bool m_globalStatsDirty = true;

    // Simulation time
    float m_totalTime = 0.0f;

    // Helper methods
    void createIsland(const IslandConfig& config, uint32_t index);
    void generateIslandTerrain(Island& island);
    void populateIsland(Island& island, unsigned int seed);

    void emitEvent(const IslandEvent& event);

    // Statistics helpers
    void updateIslandStats(Island& island);
    float calculateGeneticDiversity(const Island& island) const;
};

} // namespace Forge
