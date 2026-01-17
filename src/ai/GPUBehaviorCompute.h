#pragma once

// GPUBehaviorCompute - Extended GPU compute for multiple behavior types
// Adds specialized pipelines for flocking, predator-prey, food seeking, and migration

#include "GPUSteeringCompute.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>

// Forward declarations
class DX12Device;
class Creature;

namespace Forge {

// ============================================================================
// Behavior Type Flags (for GPU-side filtering)
// ============================================================================

enum class BehaviorType : uint32_t {
    NONE        = 0,
    FLOCKING    = 1 << 0,   // Separation, alignment, cohesion
    PREDATOR    = 1 << 1,   // Hunting, pursuit
    PREY        = 1 << 2,   // Fleeing, evasion
    FORAGING    = 1 << 3,   // Food seeking
    MIGRATION   = 1 << 4,   // Long-distance movement
    TERRITORIAL = 1 << 5,   // Territory defense
    SOCIAL      = 1 << 6,   // Group interactions
    ALL         = 0xFFFFFFFF
};

inline BehaviorType operator|(BehaviorType a, BehaviorType b) {
    return static_cast<BehaviorType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BehaviorType operator&(BehaviorType a, BehaviorType b) {
    return static_cast<BehaviorType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// ============================================================================
// Extended Input/Output Structures
// ============================================================================

// Extended creature data for behavior computation
struct alignas(16) BehaviorCreatureData {
    // Position and state (from base)
    DirectX::XMFLOAT3 position;
    float energy;

    DirectX::XMFLOAT3 velocity;
    float fear;

    // Type and status
    uint32_t type;
    uint32_t behaviorFlags;  // BehaviorType flags
    uint32_t isAlive;
    uint32_t isPredator;

    // Extended attributes
    float size;
    float speed;
    float senseRadius;
    float age;

    // Social/territorial
    uint32_t speciesID;
    uint32_t packID;
    float territoryRadius;
    float socialWeight;

    // Migration
    DirectX::XMFLOAT3 migrationTarget;
    float migrationUrgency;
};

// Extended behavior output
struct alignas(16) BehaviorOutput {
    // Primary steering force
    DirectX::XMFLOAT3 steeringForce;
    float priority;

    // Flocking components (for debugging/visualization)
    DirectX::XMFLOAT3 separationForce;
    float separationWeight;

    DirectX::XMFLOAT3 alignmentForce;
    float alignmentWeight;

    DirectX::XMFLOAT3 cohesionForce;
    float cohesionWeight;

    // Target information
    DirectX::XMFLOAT3 targetPosition;
    uint32_t targetType;  // 0=none, 1=food, 2=prey, 3=flee, 4=migration

    // State flags
    uint32_t behaviorState;  // Current active behaviors
    float urgency;           // How urgent is the current action
    float confidence;        // Confidence in current decision
    uint32_t padding;
};

// ============================================================================
// Behavior Compute Configuration
// ============================================================================

struct BehaviorComputeConfig {
    // Flocking parameters
    float separationDistance = 3.0f;
    float alignmentDistance = 8.0f;
    float cohesionDistance = 10.0f;
    float separationWeight = 1.5f;
    float alignmentWeight = 1.0f;
    float cohesionWeight = 1.0f;

    // Predator-prey parameters
    float predatorDetectionRange = 30.0f;
    float preyDetectionRange = 25.0f;
    float fleeDistance = 20.0f;
    float pursuePredictionTime = 1.5f;

    // Foraging parameters
    float foodDetectionRange = 40.0f;
    float foragingPriority = 0.8f;
    float hungerThreshold = 50.0f;

    // Migration parameters
    float migrationTriggerDistance = 100.0f;
    float migrationSpeed = 1.5f;

    // General
    float maxSpeed = 5.0f;
    float maxForce = 10.0f;
    float neighborRadius = 15.0f;
};

// ============================================================================
// Behavior Statistics
// ============================================================================

struct BehaviorComputeStats {
    uint32_t creaturesProcessed = 0;
    uint32_t flockingCount = 0;
    uint32_t predatorCount = 0;
    uint32_t preyCount = 0;
    uint32_t foragingCount = 0;
    uint32_t migratingCount = 0;

    float computeTimeMs = 0.0f;
    float dataUploadTimeMs = 0.0f;
    float readbackTimeMs = 0.0f;
};

// ============================================================================
// GPUBehaviorCompute - Extended behavior computation
// ============================================================================

class GPUBehaviorCompute {
public:
    static constexpr uint32_t MAX_CREATURES = 65536;
    static constexpr uint32_t THREAD_GROUP_SIZE = 64;

    GPUBehaviorCompute();
    ~GPUBehaviorCompute();

    // Non-copyable
    GPUBehaviorCompute(const GPUBehaviorCompute&) = delete;
    GPUBehaviorCompute& operator=(const GPUBehaviorCompute&) = delete;

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize(DX12Device* device);
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // Configuration
    void setConfig(const BehaviorComputeConfig& config) { m_config = config; }
    const BehaviorComputeConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Data Update Interface (CPU side)
    // ========================================================================

    // Prepare creature data for GPU (converts from Creature objects)
    void prepareCreatureData(const std::vector<Creature*>& creatures);

    // Update specific behavior data
    void updateFlockingData(const std::vector<glm::vec3>& neighborPositions,
                            const std::vector<glm::vec3>& neighborVelocities);

    void updatePredatorData(const std::vector<glm::vec3>& predatorPositions);

    void updateFoodData(const std::vector<glm::vec3>& foodPositions,
                        const std::vector<float>& foodAmounts);

    void updateMigrationTargets(const std::vector<glm::vec3>& targets);

    // ========================================================================
    // Compute Interface
    // ========================================================================

    // Dispatch all behavior computations
    void dispatchAll(ID3D12GraphicsCommandList* cmdList, float deltaTime);

    // Dispatch specific behavior type only
    void dispatchBehavior(ID3D12GraphicsCommandList* cmdList,
                          BehaviorType behavior, float deltaTime);

    // ========================================================================
    // Results Interface
    // ========================================================================

    // Get computed steering forces (requires GPU sync)
    void readbackResults(std::vector<BehaviorOutput>& results);

    // Get steering force for specific creature (from cached results)
    glm::vec3 getSteeringForce(size_t creatureIndex) const;
    float getPriority(size_t creatureIndex) const;
    uint32_t getActiveState(size_t creatureIndex) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const BehaviorComputeStats& getStats() const { return m_stats; }

    // ========================================================================
    // Direct Integration with GPUSteeringCompute
    // ========================================================================

    // Use existing GPUSteeringCompute for basic steering, add behavior layer
    void setBaseSteeringCompute(GPUSteeringCompute* base) { m_baseCompute = base; }
    GPUSteeringCompute* getBaseSteeringCompute() { return m_baseCompute; }

private:
    DX12Device* m_device = nullptr;
    GPUSteeringCompute* m_baseCompute = nullptr;
    bool m_initialized = false;

    BehaviorComputeConfig m_config;
    BehaviorComputeStats m_stats;

    // Input data (CPU side)
    std::vector<BehaviorCreatureData> m_creatureData;
    std::vector<BehaviorOutput> m_outputCache;

    // Helper methods
    BehaviorCreatureData convertCreature(const Creature* creature, size_t index);
    void computeFlockingCPU(float deltaTime);  // CPU fallback
    void computePredatorPreyCPU(float deltaTime);
    void computeForagingCPU(float deltaTime);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline GPUBehaviorCompute::GPUBehaviorCompute() {
    m_creatureData.reserve(MAX_CREATURES);
    m_outputCache.reserve(MAX_CREATURES);
}

inline GPUBehaviorCompute::~GPUBehaviorCompute() {
    shutdown();
}

inline glm::vec3 GPUBehaviorCompute::getSteeringForce(size_t creatureIndex) const {
    if (creatureIndex >= m_outputCache.size()) {
        return glm::vec3(0.0f);
    }
    const auto& out = m_outputCache[creatureIndex];
    return glm::vec3(out.steeringForce.x, out.steeringForce.y, out.steeringForce.z);
}

inline float GPUBehaviorCompute::getPriority(size_t creatureIndex) const {
    if (creatureIndex >= m_outputCache.size()) return 0.0f;
    return m_outputCache[creatureIndex].priority;
}

inline uint32_t GPUBehaviorCompute::getActiveState(size_t creatureIndex) const {
    if (creatureIndex >= m_outputCache.size()) return 0;
    return m_outputCache[creatureIndex].behaviorState;
}

} // namespace Forge
