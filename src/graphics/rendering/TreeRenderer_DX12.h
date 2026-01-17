#pragma once

#include "../../environment/TreeGenerator.h"
#include "../../environment/VegetationManager.h"
#include "../Frustum.h"
#include "../LODSystem.h"

// Forge Engine RHI includes
#include "RHI/RHI.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

using namespace Forge;
using namespace Forge::Math;
using namespace Forge::RHI;

// Number of frames in flight for double-buffering
static constexpr uint32_t NUM_FRAMES_IN_FLIGHT_TREE = 2;

// Maximum trees per batch for instance buffer sizing
static constexpr uint32_t MAX_TREES_PER_TYPE = 8192;
// Maximum tree draws per frame for constant buffer offsets
static constexpr uint32_t MAX_TREE_DRAWS = 16384;

// Tree instance data (per-tree transform) - 48 bytes with LOD data
#pragma pack(push, 4)
struct TreeInstanceGPU {
    float position[3];    // World position (12 bytes)
    float rotation;       // Y-axis rotation in radians (4 bytes)
    float scale[3];       // Non-uniform scale (12 bytes)
    int treeType;         // TreeType enum value (4 bytes)
    float distance;       // Distance to camera (4 bytes)
    float fadeFactor;     // LOD transition fade (0-1) (4 bytes)
    uint32_t lodLevel;    // Current LOD level (4 bytes)
    float padding;        // Align to 48 bytes (4 bytes)

    // Helper to set from TreeInstance
    void SetFromInstance(const TreeInstance& inst) {
        position[0] = inst.position.x;
        position[1] = inst.position.y;
        position[2] = inst.position.z;
        rotation = inst.rotation;
        scale[0] = inst.scale.x;
        scale[1] = inst.scale.y;
        scale[2] = inst.scale.z;
        treeType = static_cast<int>(inst.type);
        distance = 0.0f;
        fadeFactor = 1.0f;
        lodLevel = 0;
        padding = 0.0f;
    }
};
#pragma pack(pop)

static_assert(sizeof(TreeInstanceGPU) == 48, "TreeInstanceGPU must be 48 bytes");

// Tree constant buffer - 256 bytes aligned for DX12
struct alignas(256) TreeConstants {
    float viewProj[16];       // 64 bytes - View-Projection matrix
    float model[16];          // 64 bytes - Per-tree model matrix
    float cameraPos[4];       // 16 bytes
    float lightDir[4];        // 16 bytes
    float lightColor[4];      // 16 bytes
    float windParams[4];      // 16 bytes - xy=direction, z=strength, w=time
    float fogParams[4];       // 16 bytes - x=fogStart, y=fogEnd, z=density, w=fadeFactor
    float fogColor[4];        // 16 bytes - rgb=color, w=unused
    float padding[8];         // 32 bytes padding to reach 256 bytes
};

static_assert(sizeof(TreeConstants) == 256, "TreeConstants must be 256 bytes");

// DX12 vertex structure for trees (40 bytes, matches pipeline input layout)
// Position (12) + padding (4) + Normal (12) + padding (4) + TexCoord (8) = 40 bytes
struct TreeVertexDX12 {
    float position[3];
    float padding1;
    float normal[3];
    float padding2;
    float texCoord[2];  // Stores color R, G from tree generator (B is computed)
};

static_assert(sizeof(TreeVertexDX12) == 40, "TreeVertexDX12 must be 40 bytes");

// DX12 Mesh data structure for trees
struct TreeMeshDX12 {
    UniquePtr<IBuffer> vertexBuffer;
    UniquePtr<IBuffer> indexBuffer;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t vertexStride = 0;
    Vec3 boundsMin;
    Vec3 boundsMax;

    bool isValid() const {
        return vertexBuffer != nullptr && indexBuffer != nullptr && indexCount > 0;
    }
};

class TreeRendererDX12 {
public:
    TreeRendererDX12();
    ~TreeRendererDX12();

    // Initialize with device and vegetation manager
    bool init(IDevice* device, VegetationManager* vegManager);

    // Pre-generate meshes for all tree types
    void generateTreeMeshes();

    // Render all visible trees
    void render(
        ICommandList* cmdList,
        IPipeline* pipeline,
        const glm::mat4& viewProj,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    // Render trees for shadow pass (depth only)
    void renderForShadow(
        ICommandList* cmdList,
        IPipeline* shadowPipeline,
        const glm::mat4& lightViewProj
    );

    // Set the current frame index for double-buffering
    void setFrameIndex(uint32_t frameIndex) { m_frameIndex = frameIndex % NUM_FRAMES_IN_FLIGHT_TREE; }

    // Get rendering statistics
    int getRenderedTreeCount() const { return m_renderedCount; }
    int getTotalTreeCount() const { return m_totalCount; }
    int getCulledTreeCount() const { return m_culledCount; }
    int getDrawCallCount() const { return m_drawCallCount; }

    // Wind control
    void setWindDirection(const glm::vec2& dir) { m_windDirection = glm::normalize(dir); }
    void setWindStrength(float strength) { m_windStrength = strength; }
    glm::vec2 getWindDirection() const { return m_windDirection; }
    float getWindStrength() const { return m_windStrength; }

    // LOD configuration
    void setLODConfig(const LOD::LODConfig& config) { m_lodConfig = config; }
    const LOD::LODConfig& getLODConfig() const { return m_lodConfig; }

    // LOD statistics
    const LOD::LODStats& getLODStats() const { return m_lodStats; }

    // Get trees by LOD level for debugging
    int getTreeCountAtLOD(LOD::TreeLOD lod) const;

private:
    IDevice* m_device = nullptr;
    VegetationManager* m_vegManager = nullptr;

    // Upload resources
    UniquePtr<ICommandList> m_uploadCommandList;
    UniquePtr<IFence> m_uploadFence;
    u64 m_uploadFenceValue = 0;

    // Pre-generated mesh per tree type
    std::unordered_map<TreeType, TreeMeshDX12> m_treeMeshes;

    // Instance buffers for each tree type (double-buffered)
    struct TypeInstanceBuffers {
        UniquePtr<IBuffer> instanceBuffer[NUM_FRAMES_IN_FLIGHT_TREE];
    };
    std::unordered_map<TreeType, TypeInstanceBuffers> m_instanceBuffers;

    // CPU-side staging for instance data per type
    std::unordered_map<TreeType, std::vector<TreeInstanceGPU>> m_stagingInstances;

    // Constant buffer (updated per-tree, but we batch by type)
    UniquePtr<IBuffer> m_constantBuffer;

    // Current frame index for double-buffering
    uint32_t m_frameIndex = 0;

    // Statistics
    int m_renderedCount = 0;
    int m_totalCount = 0;
    int m_culledCount = 0;
    int m_drawCallCount = 0;

    // Wind parameters
    glm::vec2 m_windDirection = glm::vec2(1.0f, 0.0f);
    float m_windStrength = 0.2f;

    // LOD system
    LOD::LODConfig m_lodConfig;
    LOD::LODStats m_lodStats;
    glm::vec3 m_lastCameraPos = glm::vec3(0.0f);

    // Per-LOD instance batches for efficient rendering
    std::unordered_map<TreeType, std::vector<TreeInstanceGPU>> m_lodBatches[6]; // One per LOD::TreeLOD

    // Render batch structure
    struct RenderBatch {
        TreeType type;
        TreeMeshDX12* mesh;
        std::vector<TreeInstanceGPU> instances;
    };

    // Helper methods
    bool createMeshForType(TreeType type);
    bool createInstanceBuffer(TreeType type);
    void collectVisibleInstances(const Frustum& frustum, const glm::vec3& cameraPos);
    void renderBatch(const RenderBatch& batch, ICommandList* cmdList);
    bool uploadStaticBuffer(IBuffer* dstBuffer, const void* data, size_t size, ResourceState finalState);
    glm::mat4 buildModelMatrix(const TreeInstanceGPU& instance) const;

    // LOD helpers
    void sortInstancesByDistance();
    void updateLODStats();
};
