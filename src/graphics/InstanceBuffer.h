#pragma once

#include "ForgeEngine/ForgeEngine.h"
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <cassert>

using namespace Forge;

/**
 * @class InstanceBuffer
 * @brief Generic double-buffered instance buffer for GPU instanced rendering.
 *
 * Template class that manages per-frame instance data for instanced rendering.
 * Uses double-buffering (or N-buffering) to prevent CPU/GPU race conditions.
 *
 * @tparam T The instance data type (must be trivially copyable)
 *
 * Usage:
 * @code
 * struct MyInstanceData {
 *     float modelMatrix[16];
 *     float color[4];
 * };
 *
 * InstanceBuffer<MyInstanceData> buffer;
 * buffer.create(device, 10000, 2);  // 10K instances, double-buffered
 *
 * // Per frame:
 * buffer.clear();
 * for (auto& obj : objects) {
 *     buffer.add(obj.getInstanceData());
 * }
 * buffer.upload(frameIndex);
 * buffer.bind(cmdList, 1, frameIndex);  // Bind to slot 1
 * cmdList->DrawIndexedInstanced(indexCount, buffer.count(), 0, 0, 0);
 * @endcode
 */
template<typename T>
class InstanceBuffer {
    static_assert(std::is_trivially_copyable_v<T>,
                  "InstanceBuffer<T>: T must be trivially copyable for memcpy");

public:
    InstanceBuffer() = default;
    ~InstanceBuffer() = default;

    // Non-copyable, movable
    InstanceBuffer(const InstanceBuffer&) = delete;
    InstanceBuffer& operator=(const InstanceBuffer&) = delete;
    InstanceBuffer(InstanceBuffer&&) = default;
    InstanceBuffer& operator=(InstanceBuffer&&) = default;

    /**
     * @brief Create the instance buffer.
     *
     * @param device The ForgeEngine device for buffer creation.
     * @param maxInstances Maximum number of instances the buffer can hold.
     * @param frameCount Number of frames in flight (default 2 for double-buffering).
     * @param debugName Optional debug name prefix for the buffers.
     */
    void create(IDevice* device, size_t maxInstances, u32 frameCount = 2,
                const std::string& debugName = "InstanceBuffer") {
        assert(device != nullptr);
        assert(maxInstances > 0);
        assert(frameCount > 0);

        m_maxInstances = maxInstances;
        m_frameCount = frameCount;
        m_buffers.resize(frameCount);

        BufferDesc desc;
        desc.size = maxInstances * sizeof(T);
        desc.usage = BufferUsage::Vertex;
        desc.cpuAccess = true;  // Upload heap for per-frame updates

        for (u32 i = 0; i < frameCount; ++i) {
            desc.debugName = (debugName + "[" + std::to_string(i) + "]");
            m_buffers[i] = device->CreateBuffer(desc);
            assert(m_buffers[i] != nullptr);
        }

        m_staging.reserve(maxInstances);
        m_currentCount = 0;
    }

    /**
     * @brief Clear the staging buffer for a new frame.
     *
     * Must be called at the start of each frame before adding new instances.
     */
    void clear() {
        m_staging.clear();
        m_currentCount = 0;
    }

    /**
     * @brief Add an instance to the staging buffer.
     *
     * @param instance The instance data to add.
     * @return true if the instance was added, false if buffer is full.
     */
    bool add(const T& instance) {
        if (m_staging.size() >= m_maxInstances) {
            return false;
        }
        m_staging.push_back(instance);
        return true;
    }

    /**
     * @brief Add an instance using move semantics.
     *
     * @param instance The instance data to add (moved).
     * @return true if the instance was added, false if buffer is full.
     */
    bool add(T&& instance) {
        if (m_staging.size() >= m_maxInstances) {
            return false;
        }
        m_staging.push_back(std::move(instance));
        return true;
    }

    /**
     * @brief Upload staging data to the GPU buffer for the current frame.
     *
     * @param frameIndex The current frame index (modulo frameCount).
     */
    void upload(u32 frameIndex) {
        m_currentCount = m_staging.size();
        if (m_currentCount == 0) {
            return;
        }

        IBuffer* buffer = m_buffers[frameIndex % m_frameCount].get();
        void* mapped = buffer->Map();
        if (mapped) {
            std::memcpy(mapped, m_staging.data(), m_currentCount * sizeof(T));
            buffer->Unmap();
        }
    }

    /**
     * @brief Bind the instance buffer to a vertex buffer slot.
     *
     * @param cmdList The command list to bind on.
     * @param slot The vertex buffer slot (typically 1 for instance data).
     * @param frameIndex The current frame index.
     */
    void bind(IGraphicsCommandList* cmdList, u32 slot, u32 frameIndex) const {
        assert(cmdList != nullptr);
        IBuffer* buffer = m_buffers[frameIndex % m_frameCount].get();
        cmdList->BindVertexBuffer(slot, buffer, static_cast<u32>(sizeof(T)), 0);
    }

    /**
     * @brief Get the current number of instances.
     *
     * @return The number of instances uploaded in the last upload() call.
     */
    size_t count() const { return m_currentCount; }

    /**
     * @brief Get the maximum capacity.
     *
     * @return The maximum number of instances this buffer can hold.
     */
    size_t capacity() const { return m_maxInstances; }

    /**
     * @brief Check if the buffer is empty.
     *
     * @return true if no instances have been uploaded.
     */
    bool empty() const { return m_currentCount == 0; }

    /**
     * @brief Get the raw GPU buffer for a specific frame.
     *
     * @param frameIndex The frame index.
     * @return Pointer to the GPU buffer.
     */
    IBuffer* getBuffer(u32 frameIndex) const {
        return m_buffers[frameIndex % m_frameCount].get();
    }

    /**
     * @brief Get the size of each instance in bytes.
     *
     * @return sizeof(T)
     */
    static constexpr size_t instanceSize() { return sizeof(T); }

    /**
     * @brief Get the total GPU memory used by this buffer.
     *
     * @return Total bytes across all frame buffers.
     */
    size_t totalMemoryBytes() const {
        return m_maxInstances * sizeof(T) * m_frameCount;
    }

    /**
     * @brief Get access to the staging vector for direct manipulation.
     *
     * Use with caution - prefer add() for normal usage.
     *
     * @return Reference to the staging vector.
     */
    std::vector<T>& getStagingVector() { return m_staging; }
    const std::vector<T>& getStagingVector() const { return m_staging; }

private:
    std::vector<UniquePtr<IBuffer>> m_buffers;
    std::vector<T> m_staging;
    size_t m_maxInstances = 0;
    size_t m_currentCount = 0;
    u32 m_frameCount = 2;
};

/**
 * @struct CreatureInstanceData
 * @brief Standard instance data for creature rendering.
 *
 * Contains model matrix (as 4 float4 rows) and color/type information.
 * Total size: 80 bytes (5 x float4).
 */
#pragma pack(push, 4)
struct CreatureInstanceData {
    float modelRow0[4];  // 16 bytes - Model matrix row 0
    float modelRow1[4];  // 16 bytes - Model matrix row 1
    float modelRow2[4];  // 16 bytes - Model matrix row 2
    float modelRow3[4];  // 16 bytes - Model matrix row 3
    float colorType[4];  // 16 bytes - RGB color + creature type/LOD

    void setModelMatrix(const float* m) {
        std::memcpy(modelRow0, m, 16);
        std::memcpy(modelRow1, m + 4, 16);
        std::memcpy(modelRow2, m + 8, 16);
        std::memcpy(modelRow3, m + 12, 16);
    }

    void setColor(float r, float g, float b, float typeOrLOD = 0.0f) {
        colorType[0] = r;
        colorType[1] = g;
        colorType[2] = b;
        colorType[3] = typeOrLOD;
    }
};
#pragma pack(pop)

static_assert(sizeof(CreatureInstanceData) == 80, "CreatureInstanceData must be 80 bytes");

/**
 * @struct TreeInstanceData
 * @brief Instance data for tree/vegetation rendering.
 *
 * Same layout as CreatureInstanceData for shader compatibility.
 */
#pragma pack(push, 4)
struct TreeInstanceData {
    float modelRow0[4];  // 16 bytes
    float modelRow1[4];  // 16 bytes
    float modelRow2[4];  // 16 bytes
    float modelRow3[4];  // 16 bytes
    float colorType[4];  // 16 bytes - RGB + tree type index

    void setModelMatrix(const float* m) {
        std::memcpy(modelRow0, m, 16);
        std::memcpy(modelRow1, m + 4, 16);
        std::memcpy(modelRow2, m + 8, 16);
        std::memcpy(modelRow3, m + 12, 16);
    }

    void setColor(float r, float g, float b, float treeType = 0.0f) {
        colorType[0] = r;
        colorType[1] = g;
        colorType[2] = b;
        colorType[3] = treeType;
    }
};
#pragma pack(pop)

static_assert(sizeof(TreeInstanceData) == 80, "TreeInstanceData must be 80 bytes");

/**
 * @struct BillboardInstanceData
 * @brief Instance data for billboard/impostor rendering (LOD 2).
 *
 * Compact format for distant objects rendered as camera-facing quads.
 */
#pragma pack(push, 4)
struct BillboardInstanceData {
    float position[3];   // 12 bytes - World position
    float size;          // 4 bytes  - Billboard size
    float color[4];      // 16 bytes - RGBA color
    float texCoords[4];  // 16 bytes - UV rect in atlas (u0, v0, u1, v1)
};
#pragma pack(pop)

static_assert(sizeof(BillboardInstanceData) == 48, "BillboardInstanceData must be 48 bytes");
