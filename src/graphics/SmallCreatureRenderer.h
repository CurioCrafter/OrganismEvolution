#pragma once

#include "../entities/small/SmallCreatures.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

namespace small {

using namespace DirectX;

// Forward declarations
class SmallCreatureManager;
struct PheromonePoint;

// LOD levels for rendering
enum class LODLevel : uint8_t {
    DETAILED,    // Full model, < 10m
    SIMPLIFIED,  // Low-poly, 10-30m
    POINT,       // Point sprite, 30-100m
    PARTICLE     // Swarm particle, > 100m
};

// Instance data for GPU instanced rendering
struct SmallCreatureInstance {
    XMFLOAT4X4 world;           // World transform
    XMFLOAT4 color;             // RGBA color
    XMFLOAT4 params;            // x=scale, y=animation, z=type, w=lod
};

// Point sprite instance for distant rendering
struct PointSpriteInstance {
    XMFLOAT3 position;
    float size;
    XMFLOAT4 color;
};

// Swarm particle for very distant/group rendering
struct SwarmParticle {
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    float size;
    float life;
    XMFLOAT4 color;
};

// Trail segment for pheromone visualization
struct TrailSegment {
    XMFLOAT3 start;
    XMFLOAT3 end;
    float strength;
    XMFLOAT4 color;
};

// Mesh data for creature type
struct CreatureMesh {
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;
    uint32_t indexCount;
    uint32_t vertexCount;
};

// Render batch for instanced drawing
struct RenderBatch {
    SmallCreatureType type;
    LODLevel lod;
    std::vector<SmallCreatureInstance> instances;
    uint32_t startIndex;
    uint32_t instanceCount;
};

// Main renderer class
class SmallCreatureRenderer {
public:
    SmallCreatureRenderer();
    ~SmallCreatureRenderer();

    // Initialize with D3D12 device
    bool initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    // Prepare for rendering (call each frame before render)
    void prepareFrame(const XMFLOAT3& cameraPos, const XMFLOAT4X4& viewProj);

    // Build render data from creature manager
    void buildRenderData(SmallCreatureManager& manager);

    // Render all small creatures
    void render(ID3D12GraphicsCommandList* cmdList);

    // Render pheromone trails (debug/visualization)
    void renderPheromoneTrails(ID3D12GraphicsCommandList* cmdList,
                                const std::vector<PheromonePoint>& pheromones);

    // Settings
    void setMaxRenderDistance(float dist) { maxRenderDistance_ = dist; }
    void setDetailedLODDistance(float dist) { detailedLODDist_ = dist; }
    void setSimplifiedLODDistance(float dist) { simplifiedLODDist_ = dist; }
    void setPointLODDistance(float dist) { pointLODDist_ = dist; }
    void setShowPheromones(bool show) { showPheromones_ = show; }

    // Statistics
    size_t getTotalInstanceCount() const { return totalInstances_; }
    size_t getDrawCallCount() const { return batches_.size(); }
    size_t getDetailedCount() const { return detailedCount_; }
    size_t getPointCount() const { return pointCount_; }
    size_t getParticleCount() const { return particleCount_; }

    // Habitat density stats (for debug overlay)
    struct HabitatStats {
        size_t groundCount = 0;       // On ground surface
        size_t aerialCount = 0;       // Flying in air
        size_t canopyCount = 0;       // In trees
        size_t undergroundCount = 0;  // Below ground (approximated)
        size_t aquaticCount = 0;      // In/on water
        size_t grassCount = 0;        // In grass layer
        size_t bushCount = 0;         // In bush/shrub layer

        // Size category distribution
        size_t microscopicCount = 0;  // < 1mm
        size_t tinyCount = 0;         // 1mm - 1cm
        size_t smallCount = 0;        // 1cm - 10cm
        size_t mediumCount = 0;       // 10cm - 30cm
    };
    const HabitatStats& getHabitatStats() const { return habitatStats_; }

    // Get formatted debug string for habitat distribution
    std::string getHabitatDebugString() const;

    // Debug settings
    void setShowHabitatDebug(bool show) { showHabitatDebug_ = show; }

private:
    // Create meshes for each creature type
    void createCreatureMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    // Create procedural mesh for creature type
    void createInsectMesh(ID3D12Device* device, SmallCreatureType type);
    void createArachnidMesh(ID3D12Device* device, SmallCreatureType type);
    void createSmallMammalMesh(ID3D12Device* device, SmallCreatureType type);
    void createReptileMesh(ID3D12Device* device, SmallCreatureType type);
    void createAmphibianMesh(ID3D12Device* device, SmallCreatureType type);

    // Create shaders
    void createShaders(ID3D12Device* device);

    // Create instance buffers
    void createInstanceBuffers(ID3D12Device* device);

    // Upload instance data to GPU
    void uploadInstanceData(ID3D12GraphicsCommandList* cmdList);

    // Sort and batch creatures for rendering
    void sortAndBatch();

    // Calculate LOD for creature at distance
    LODLevel calculateLOD(float distance) const;

    // Get mesh for creature type
    CreatureMesh* getMesh(SmallCreatureType type, LODLevel lod);

    // Get mesh scale factor for converting unit-scale meshes to world scale
    // Meshes are authored at ~1 unit, this converts to proper world meters
    float getMeshScaleFactor(SmallCreatureType type) const;

    // Get visibility bias for point sprites by size category
    // Allows tiny creatures to remain visible at distance without being oversized
    float getVisibilityBias(SizeCategory category) const;

    // D3D12 resources
    ID3D12Device* device_;

    // Shaders and pipeline state
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> instancedPSO_;
    ComPtr<ID3D12PipelineState> pointSpritePSO_;
    ComPtr<ID3D12PipelineState> particlePSO_;
    ComPtr<ID3D12PipelineState> trailPSO_;

    // Meshes by type
    std::unordered_map<int, CreatureMesh> meshes_;
    std::unordered_map<int, CreatureMesh> simplifiedMeshes_;

    // Instance buffers
    ComPtr<ID3D12Resource> instanceBuffer_;
    ComPtr<ID3D12Resource> instanceUploadBuffer_;
    ComPtr<ID3D12Resource> pointSpriteBuffer_;
    ComPtr<ID3D12Resource> particleBuffer_;
    ComPtr<ID3D12Resource> trailBuffer_;

    static constexpr size_t MAX_INSTANCES = 50000;
    static constexpr size_t MAX_POINT_SPRITES = 100000;
    static constexpr size_t MAX_PARTICLES = 50000;
    static constexpr size_t MAX_TRAIL_SEGMENTS = 10000;

    // Render data
    std::vector<SmallCreatureInstance> instances_;
    std::vector<PointSpriteInstance> pointSprites_;
    std::vector<SwarmParticle> particles_;
    std::vector<TrailSegment> trails_;
    std::vector<RenderBatch> batches_;

    // Camera data
    XMFLOAT3 cameraPos_;
    XMFLOAT4X4 viewProj_;

    // LOD distances
    float maxRenderDistance_ = 200.0f;
    float detailedLODDist_ = 10.0f;
    float simplifiedLODDist_ = 30.0f;
    float pointLODDist_ = 100.0f;

    // Settings
    bool showPheromones_ = false;
    bool showHabitatDebug_ = false;

    // Statistics
    size_t totalInstances_ = 0;
    size_t detailedCount_ = 0;
    size_t simplifiedCount_ = 0;
    size_t pointCount_ = 0;
    size_t particleCount_ = 0;

    // Habitat statistics
    HabitatStats habitatStats_;
};

// Helper class for swarm visualization
class SwarmVisualizer {
public:
    SwarmVisualizer();
    ~SwarmVisualizer();

    // Initialize
    bool initialize(ID3D12Device* device);

    // Update particle system
    void update(float deltaTime, const std::vector<SmallCreature>& creatures);

    // Get particles for rendering
    const std::vector<SwarmParticle>& getParticles() const { return particles_; }

    // Generate swarm particles from creature group
    void generateSwarmParticles(const XMFLOAT3& center, int count,
                                 SmallCreatureType type, const XMFLOAT4& color);

private:
    std::vector<SwarmParticle> particles_;
    std::mt19937 rng_;
};

// Helper for generating procedural creature meshes
class ProceduralMeshGenerator {
public:
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 texCoord;
    };

    // Generate insect body (segmented ellipsoids)
    static void generateInsectBody(std::vector<Vertex>& vertices,
                                   std::vector<uint16_t>& indices,
                                   float headSize, float thoraxSize, float abdomenSize);

    // Generate spider body (two segments, 8 legs)
    static void generateSpiderBody(std::vector<Vertex>& vertices,
                                   std::vector<uint16_t>& indices,
                                   float bodySize);

    // Generate small mammal (furry ellipsoid with tail)
    static void generateMammalBody(std::vector<Vertex>& vertices,
                                   std::vector<uint16_t>& indices,
                                   float bodyLength, float bodyWidth);

    // Generate frog (squat body with legs)
    static void generateFrogBody(std::vector<Vertex>& vertices,
                                  std::vector<uint16_t>& indices,
                                  float bodySize);

    // Generate simplified versions (low poly)
    static void generateSimplifiedInsect(std::vector<Vertex>& vertices,
                                          std::vector<uint16_t>& indices);
    static void generateSimplifiedQuadruped(std::vector<Vertex>& vertices,
                                             std::vector<uint16_t>& indices);

private:
    static void addEllipsoid(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices,
                             const XMFLOAT3& center, const XMFLOAT3& radii,
                             int segments, int rings);
    static void addCylinder(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices,
                            const XMFLOAT3& start, const XMFLOAT3& end, float radius,
                            int segments);
    static void addCone(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices,
                        const XMFLOAT3& base, const XMFLOAT3& tip, float radius, int segments);
};

} // namespace small
