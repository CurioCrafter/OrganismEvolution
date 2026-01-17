#pragma once

#include "CreatureMeshCache.h"
#include "../../entities/Creature.h"
#include "../Camera.h"
#include "../Frustum.h"

// Forge Engine RHI includes
#include "RHI/RHI.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

#include <vector>
#include <memory>
#include <cstdint>
#include <cstddef>

using namespace Forge;
using namespace Forge::Math;
using namespace Forge::RHI;

// Number of frames in flight for double-buffering
static constexpr uint32_t NUM_FRAMES_IN_FLIGHT_CREATURE = 2;

// Maximum creatures per batch for instance buffer sizing
static constexpr uint32_t MAX_CREATURES_PER_BATCH = 2000;

// Per-instance data sent to GPU (must match HLSL layout)
// Total: 80 bytes per instance (5 x float4)
#pragma pack(push, 4)
struct CreatureInstanceDataDX12 {
    float modelRow0[4];  // Model matrix row 0 (16 bytes)
    float modelRow1[4];  // Model matrix row 1 (16 bytes)
    float modelRow2[4];  // Model matrix row 2 (16 bytes)
    float modelRow3[4];  // Model matrix row 3 (16 bytes)
    float colorData[4];  // RGB color + animation phase in W (16 bytes)

    // Helper to set from glm::mat4 and color
    void SetFromCreature(const glm::mat4& model, const glm::vec3& color, float animPhase) {
        // GLM is column-major, HLSL float4x4 constructor takes rows
        // Need to transpose for correct interpretation
        modelRow0[0] = model[0][0]; modelRow0[1] = model[1][0];
        modelRow0[2] = model[2][0]; modelRow0[3] = model[3][0];

        modelRow1[0] = model[0][1]; modelRow1[1] = model[1][1];
        modelRow1[2] = model[2][1]; modelRow1[3] = model[3][1];

        modelRow2[0] = model[0][2]; modelRow2[1] = model[1][2];
        modelRow2[2] = model[2][2]; modelRow2[3] = model[3][2];

        modelRow3[0] = model[0][3]; modelRow3[1] = model[1][3];
        modelRow3[2] = model[2][3]; modelRow3[3] = model[3][3];

        // Color and animation phase
        colorData[0] = color.x;
        colorData[1] = color.y;
        colorData[2] = color.z;
        colorData[3] = animPhase;
    }
};
#pragma pack(pop)

// Validate instance data layout
static_assert(sizeof(CreatureInstanceDataDX12) == 80, "CreatureInstanceDataDX12 must be 80 bytes");
static_assert(offsetof(CreatureInstanceDataDX12, modelRow0) == 0, "modelRow0 must be at offset 0");
static_assert(offsetof(CreatureInstanceDataDX12, modelRow1) == 16, "modelRow1 must be at offset 16");
static_assert(offsetof(CreatureInstanceDataDX12, modelRow2) == 32, "modelRow2 must be at offset 32");
static_assert(offsetof(CreatureInstanceDataDX12, modelRow3) == 48, "modelRow3 must be at offset 48");
static_assert(offsetof(CreatureInstanceDataDX12, colorData) == 64, "colorData must be at offset 64");

// DX12 Mesh data structure (replaces OpenGL VAO/VBO/EBO)
struct MeshDataDX12 {
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

class CreatureRendererDX12 {
public:
    CreatureRendererDX12();
    ~CreatureRendererDX12();

    // Initialize with device and mesh cache
    bool init(IDevice* device, CreatureMeshCache* cache);

    // Render all creatures
    void render(
        const std::vector<std::unique_ptr<Creature>>& creatures,
        const Camera& camera,
        ICommandList* cmdList,
        IPipeline* pipeline,
        float time
    );

    // Render creatures for shadow pass (depth only, no lighting)
    void renderForShadow(
        const std::vector<std::unique_ptr<Creature>>& creatures,
        ICommandList* cmdList,
        IPipeline* shadowPipeline,
        float time
    );

    // Set the current frame index for double-buffering
    void setFrameIndex(uint32_t frameIndex) { m_frameIndex = frameIndex % NUM_FRAMES_IN_FLIGHT_CREATURE; }

    // Get rendering statistics
    int getLastDrawCalls() const { return m_lastDrawCalls; }
    int getLastInstancesRendered() const { return m_lastInstancesRendered; }
    int getLastCulledCount() const { return m_lastCulledCount; }

private:
    IDevice* m_device;
    CreatureMeshCache* m_meshCache;
    UniquePtr<ICommandList> m_uploadCommandList;
    UniquePtr<IFence> m_uploadFence;
    u64 m_uploadFenceValue = 0;

    // Per-frame instance buffers (double-buffered to prevent CPU/GPU race)
    // One per mesh key for batched rendering
    struct BatchInstanceBuffers {
        UniquePtr<IBuffer> instanceBuffer[NUM_FRAMES_IN_FLIGHT_CREATURE];
    };
    std::unordered_map<MeshKey, BatchInstanceBuffers> m_instanceBuffers;

    // DX12 mesh cache (converted from OpenGL MeshData)
    std::unordered_map<MeshKey, MeshDataDX12> m_dx12MeshCache;

    // Current frame index for double-buffering
    uint32_t m_frameIndex = 0;

    // Statistics
    int m_lastDrawCalls = 0;
    int m_lastInstancesRendered = 0;
    int m_lastCulledCount = 0;

    // Group creatures by mesh key
    struct RenderBatch {
        MeshKey key;
        MeshDataDX12* mesh;
        std::vector<CreatureInstanceDataDX12> instances;
    };

    // CPU-side staging buffers (reused each frame)
    std::unordered_map<MeshKey, std::vector<CreatureInstanceDataDX12>> m_stagingBuffers;

    // Helper methods
    bool createInstanceBuffer(const MeshKey& key);
    MeshDataDX12* getOrCreateDX12Mesh(const MeshKey& key, const Genome& genome, CreatureType type);
    void renderBatch(const RenderBatch& batch, ICommandList* cmdList, int creatureType);
    bool uploadStaticBuffer(IBuffer* dstBuffer, const void* data, size_t size, ResourceState finalState);
};
