#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

// Forward declarations for DX12
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12DescriptorHeap;

namespace aquatic {

// ============================================================================
// Schooling Configuration
// ============================================================================

struct SchoolingConfig {
    // Separation - avoid crowding neighbors
    float separationRadius = 2.0f;      // Distance to start avoiding
    float separationWeight = 1.5f;      // How strongly to avoid
    float separationMaxForce = 3.0f;    // Maximum separation force

    // Alignment - steer towards average heading of neighbors
    float alignmentRadius = 8.0f;       // Radius to check for alignment
    float alignmentWeight = 1.0f;       // How strongly to align
    float alignmentMaxForce = 2.0f;     // Maximum alignment force

    // Cohesion - steer towards center of mass of neighbors
    float cohesionRadius = 12.0f;       // Radius to check for cohesion
    float cohesionWeight = 0.8f;        // How strongly to group
    float cohesionMaxForce = 2.0f;      // Maximum cohesion force

    // School-wide behavior
    float preferredSchoolSize = 50.0f;
    float schoolMergeDistance = 20.0f;  // Distance to merge schools
    float schoolSplitSize = 200.0f;     // Split school if too large

    // Predator avoidance
    float predatorDetectionRange = 25.0f;
    float predatorFleeForce = 5.0f;
    float panicSpreadRadius = 8.0f;     // How far panic spreads
    float panicDecayRate = 0.5f;        // How fast panic fades

    // Movement constraints
    float maxSpeed = 10.0f;
    float maxAcceleration = 8.0f;
    float minSpeed = 2.0f;              // Fish don't stop completely
    float turnRate = 4.0f;              // Radians per second

    // Depth behavior
    float preferredDepth = 10.0f;
    float depthVariation = 5.0f;
    float verticalCorrectionForce = 2.0f;

    // Random wandering
    float wanderStrength = 0.5f;
    float wanderRadius = 2.0f;
    float wanderJitter = 0.3f;

    // Energy
    float schoolingEnergyBonus = 0.2f;  // Reduced energy use in school
};

// ============================================================================
// Individual Fish State (GPU-friendly struct)
// ============================================================================

struct alignas(16) FishState {
    glm::vec3 position;
    float padding1;

    glm::vec3 velocity;
    float speed;

    glm::vec3 acceleration;
    float padding2;

    glm::vec3 forward;
    float padding3;

    uint32_t schoolId;
    uint32_t speciesId;
    float energy;
    float panicLevel;

    float swimPhase;
    float targetDepth;
    float age;
    uint32_t flags;     // Bit flags for state

    // Per-fish genome traits that affect schooling
    float separationWeight;
    float alignmentWeight;
    float cohesionWeight;
    float maxSpeed;
};

static_assert(sizeof(FishState) == 96, "FishState must be 96 bytes for GPU alignment");

// ============================================================================
// School Data Structure
// ============================================================================

struct School {
    uint32_t schoolId = 0;
    uint32_t speciesId = 0;

    glm::vec3 centerOfMass = glm::vec3(0.0f);
    glm::vec3 averageVelocity = glm::vec3(0.0f);
    glm::vec3 averageForward = glm::vec3(1.0f, 0.0f, 0.0f);

    float averageDepth = 0.0f;
    float schoolRadius = 0.0f;      // Bounding radius
    float density = 0.0f;           // Fish per unit volume

    uint32_t fishCount = 0;
    uint32_t firstFishIndex = 0;    // Index in fish buffer

    float panicLevel = 0.0f;        // School-wide panic
    float cohesionLevel = 1.0f;     // How tight the school is

    // Behavior state
    enum class State : uint8_t {
        CRUISING,
        FEEDING,
        FLEEING,
        MIGRATING,
        SPAWNING
    } state = State::CRUISING;

    glm::vec3 targetPosition = glm::vec3(0.0f);
    float stateTimer = 0.0f;
};

// ============================================================================
// GPU Compute Shader Constants
// ============================================================================

struct alignas(256) SchoolingConstants {
    // Schooling parameters
    float separationRadius;
    float separationWeight;
    float alignmentRadius;
    float alignmentWeight;

    float cohesionRadius;
    float cohesionWeight;
    float predatorDetectionRange;
    float predatorFleeForce;

    float maxSpeed;
    float maxAcceleration;
    float minSpeed;
    float turnRate;

    float wanderStrength;
    float wanderRadius;
    float panicDecayRate;
    float deltaTime;

    // Environment
    float waterSurfaceY;
    float seaFloorY;
    float preferredDepth;
    float depthVariation;

    glm::vec3 currentDirection;
    float currentStrength;

    // Counts
    uint32_t fishCount;
    uint32_t predatorCount;
    uint32_t foodSourceCount;
    uint32_t padding;

    // Random seed
    uint32_t randomSeed;
    uint32_t frameNumber;
    float time;
    float padding2;
};

// ============================================================================
// Spatial Grid for O(n) neighbor finding
// ============================================================================

struct SpatialGridCell {
    uint32_t startIndex = 0;
    uint32_t count = 0;
};

class SpatialHashGrid {
public:
    SpatialHashGrid(float cellSize = 5.0f, uint32_t gridSize = 256);

    void clear();
    void insert(uint32_t fishIndex, const glm::vec3& position);
    void build(const std::vector<FishState>& fish);

    std::vector<uint32_t> queryNeighbors(const glm::vec3& position, float radius) const;
    void queryNeighborsIntoBuffer(const glm::vec3& position, float radius,
                                  std::vector<uint32_t>& outIndices) const;

    uint32_t getCellIndex(const glm::vec3& position) const;

    float getCellSize() const { return m_cellSize; }
    uint32_t getGridSize() const { return m_gridSize; }
    const std::vector<SpatialGridCell>& getCells() const { return m_cells; }
    const std::vector<uint32_t>& getIndices() const { return m_indices; }

private:
    float m_cellSize;
    uint32_t m_gridSize;
    glm::vec3 m_gridMin;
    glm::vec3 m_gridMax;

    std::vector<SpatialGridCell> m_cells;
    std::vector<uint32_t> m_indices;
    std::vector<std::pair<uint32_t, uint32_t>> m_sortBuffer; // (cellIndex, fishIndex)
};

// ============================================================================
// Fish Schooling Manager
// ============================================================================

class FishSchoolingManager {
public:
    FishSchoolingManager();
    ~FishSchoolingManager();

    // Initialization
    bool initialize(ID3D12Device* device, uint32_t maxFish = 10000);
    void shutdown();

    // Fish management
    uint32_t addFish(const FishState& fish);
    void removeFish(uint32_t index);
    void clearAllFish();

    // School management
    uint32_t createSchool(uint32_t speciesId, const glm::vec3& center);
    void dissolveSchool(uint32_t schoolId);
    void mergeSchools(uint32_t schoolA, uint32_t schoolB);
    void updateSchools();

    // Predator/threat management
    void addPredatorPosition(const glm::vec3& position, float threatLevel = 1.0f);
    void clearPredators();

    // Food source management
    void addFoodSource(const glm::vec3& position, float value = 1.0f);
    void clearFoodSources();

    // Update - CPU fallback or GPU dispatch
    void update(float deltaTime);
    void updateCPU(float deltaTime);
    void updateGPU(ID3D12GraphicsCommandList* commandList, float deltaTime);

    // Sync GPU results back to CPU
    void syncFromGPU(ID3D12GraphicsCommandList* commandList);

    // Accessors
    const std::vector<FishState>& getFish() const { return m_fish; }
    std::vector<FishState>& getFish() { return m_fish; }
    const std::vector<School>& getSchools() const { return m_schools; }

    FishState* getFishPtr(uint32_t index);
    const FishState* getFishPtr(uint32_t index) const;

    uint32_t getFishCount() const { return static_cast<uint32_t>(m_fish.size()); }
    uint32_t getSchoolCount() const { return static_cast<uint32_t>(m_schools.size()); }
    uint32_t getMaxFish() const { return m_maxFish; }

    // Configuration
    void setConfig(const SchoolingConfig& config) { m_config = config; }
    const SchoolingConfig& getConfig() const { return m_config; }

    void setWaterBounds(float surfaceY, float floorY);
    void setCurrentDirection(const glm::vec3& direction, float strength);

    // GPU resources for instanced rendering
    ID3D12Resource* getFishBuffer() const { return m_fishBuffer; }
    uint32_t getFishBufferStride() const { return sizeof(FishState); }

    // Debug/visualization
    void debugDrawSchools() const;
    void getSchoolStatistics(uint32_t schoolId, float& outDensity,
                            float& outCohesion, float& outPanicLevel) const;

private:
    // CPU simulation
    glm::vec3 calculateSeparation(uint32_t fishIndex, const std::vector<uint32_t>& neighbors);
    glm::vec3 calculateAlignment(uint32_t fishIndex, const std::vector<uint32_t>& neighbors);
    glm::vec3 calculateCohesion(uint32_t fishIndex, const std::vector<uint32_t>& neighbors);
    glm::vec3 calculatePredatorAvoidance(uint32_t fishIndex);
    glm::vec3 calculateFoodAttraction(uint32_t fishIndex);
    glm::vec3 calculateWander(uint32_t fishIndex, float deltaTime);
    glm::vec3 calculateBoundaryAvoidance(uint32_t fishIndex);
    glm::vec3 calculateDepthCorrection(uint32_t fishIndex);

    void integrateMotion(uint32_t fishIndex, const glm::vec3& steering, float deltaTime);
    void updateSwimAnimation(uint32_t fishIndex, float deltaTime);
    void propagatePanic(float deltaTime);
    void assignToSchools();

    // GPU setup
    bool createComputePipeline(ID3D12Device* device);
    bool createBuffers(ID3D12Device* device);
    void updateConstantBuffer();

    // Data
    std::vector<FishState> m_fish;
    std::vector<School> m_schools;
    std::unordered_map<uint32_t, uint32_t> m_schoolMap;  // schoolId -> index

    std::vector<glm::vec4> m_predators;    // xyz=position, w=threatLevel
    std::vector<glm::vec4> m_foodSources;  // xyz=position, w=value

    SchoolingConfig m_config;
    SpatialHashGrid m_spatialGrid;

    // Environment
    float m_waterSurfaceY = 0.0f;
    float m_seaFloorY = -100.0f;
    glm::vec3 m_currentDirection = glm::vec3(0.0f);
    float m_currentStrength = 0.0f;

    // State
    uint32_t m_maxFish = 10000;
    uint32_t m_frameNumber = 0;
    float m_time = 0.0f;
    bool m_useGPU = false;

    // DX12 resources
    ID3D12Device* m_device = nullptr;
    ID3D12RootSignature* m_rootSignature = nullptr;
    ID3D12PipelineState* m_computePipeline = nullptr;
    ID3D12DescriptorHeap* m_descriptorHeap = nullptr;

    ID3D12Resource* m_fishBuffer = nullptr;         // UAV
    ID3D12Resource* m_fishReadbackBuffer = nullptr; // For CPU sync
    ID3D12Resource* m_predatorBuffer = nullptr;
    ID3D12Resource* m_foodBuffer = nullptr;
    ID3D12Resource* m_constantBuffer = nullptr;
    ID3D12Resource* m_gridBuffer = nullptr;         // Spatial hash on GPU

    SchoolingConstants m_constants;
};

// ============================================================================
// Schooling Behavior Utilities
// ============================================================================

namespace SchoolingUtils {

// Formation patterns
enum class FormationType {
    SPHERE,     // Default - spherical school
    TORNADO,    // Rotating column
    FLAT,       // Horizontal disc
    STREAM,     // Elongated stream
    V_FORMATION // Birds-like V
};

glm::vec3 calculateFormationOffset(FormationType type, uint32_t fishIndex,
                                   uint32_t totalFish, float baseRadius);

// Panic behaviors
float calculatePanicFromPredator(const glm::vec3& fishPos, const glm::vec3& predatorPos,
                                 float detectionRange);

// School identification
bool shouldMergeSchools(const School& a, const School& b,
                       float mergeDistance, bool sameSpecies);

// Energy calculations
float calculateSwimmingEnergyCost(float speed, float maxSpeed, bool inSchool,
                                 float schoolEnergyBonus);

// Random motion
glm::vec3 calculateWanderTarget(const glm::vec3& currentTarget,
                               float wanderRadius, float jitter, uint32_t seed);

} // namespace SchoolingUtils

// ============================================================================
// Extended School Group Dynamics (Phase 7 Behavior Variety)
// ============================================================================

/**
 * @brief Extended school behavior state for group dynamics
 */
enum class SchoolBehaviorState {
    CRUISING,           // Normal movement
    FEEDING_FRENZY,     // Aggressive feeding when food found
    PANIC_SCATTER,      // Split due to predator
    REFORMING,          // Rejoining after scatter
    LEADER_FOLLOWING,   // Following designated leader
    DEPTH_MIGRATION,    // Vertical movement for temperature/food
};

/**
 * @brief School dynamics controller for advanced group behaviors
 */
struct SchoolDynamics {
    SchoolBehaviorState state = SchoolBehaviorState::CRUISING;
    float stateStartTime = 0.0f;

    // Leader system
    uint32_t leaderId = 0;
    float leaderScore = 0.0f;      // Based on experience/size
    bool hasDesignatedLeader = false;

    // Split/rejoin mechanics
    std::vector<glm::vec3> splitPositions;  // Where groups split to
    float rejoinTimer = 0.0f;
    float splitDistance = 20.0f;

    // Panic wave
    glm::vec3 panicOrigin = glm::vec3(0.0f);
    float panicWaveRadius = 0.0f;
    float panicWaveSpeed = 15.0f;

    // Schooling intensity modulation
    float intensityMultiplier = 1.0f;  // 0.5 = loose, 2.0 = tight

    void update(float deltaTime);
    void triggerPanicWave(const glm::vec3& origin, float time);
    void requestSplit(int numGroups);
    void requestRejoin();
    std::string getStateName() const;
};

/**
 * @brief Calculate leader following force for schooling fish
 */
glm::vec3 calculateLeaderFollowForce(const glm::vec3& fishPos, const glm::vec3& leaderPos,
                                     const glm::vec3& leaderVel, float followDistance);

/**
 * @brief Calculate panic wave propagation force
 */
glm::vec3 calculatePanicWaveForce(const glm::vec3& fishPos, const glm::vec3& panicOrigin,
                                  float waveRadius, float waveIntensity);

/**
 * @brief Calculate leader score based on fish attributes
 */
float calculateLeaderScore(float age, float size, float energy, float survivalTime);

} // namespace aquatic
