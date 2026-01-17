#include "CreatureRenderer_DX12.h"
#include "../mesh/MeshData.h"
#include <unordered_map>
#include <iostream>
#include <cstring>

// DX12 vertex structure (must match pipeline input layout)
// Position (12) + padding (4) + Normal (12) + padding (4) + TexCoord (8) = 40 bytes
struct CreatureVertexDX12 {
    float position[3];
    float padding1;
    float normal[3];
    float padding2;
    float texCoord[2];
};

static_assert(sizeof(CreatureVertexDX12) == 40, "CreatureVertexDX12 must be 40 bytes");

CreatureRendererDX12::CreatureRendererDX12()
    : m_device(nullptr)
    , m_meshCache(nullptr)
    , m_frameIndex(0)
    , m_lastDrawCalls(0)
    , m_lastInstancesRendered(0)
    , m_lastCulledCount(0)
{
}

CreatureRendererDX12::~CreatureRendererDX12() {
    // UniquePtr handles cleanup automatically
    m_instanceBuffers.clear();
    m_dx12MeshCache.clear();
}

bool CreatureRendererDX12::init(IDevice* device, CreatureMeshCache* cache) {
    m_device = device;
    m_meshCache = cache;

    if (!m_device || !m_meshCache) {
        std::cerr << "[CreatureRendererDX12] ERROR: Null device or mesh cache!" << std::endl;
        return false;
    }

    m_uploadCommandList = m_device->CreateCommandList(CommandListType::Graphics);
    if (!m_uploadCommandList) {
        std::cerr << "[CreatureRendererDX12] ERROR: Failed to create upload command list!" << std::endl;
        return false;
    }

    m_uploadFence = m_device->CreateFence(0);
    if (!m_uploadFence) {
        std::cerr << "[CreatureRendererDX12] ERROR: Failed to create upload fence!" << std::endl;
        return false;
    }

    std::cout << "[CreatureRendererDX12] Initialized" << std::endl;
    return true;
}

bool CreatureRendererDX12::createInstanceBuffer(const MeshKey& key) {
    if (m_instanceBuffers.find(key) != m_instanceBuffers.end()) {
        return true; // Already created
    }

    BatchInstanceBuffers buffers;

    BufferDesc instBufDesc;
    instBufDesc.size = MAX_CREATURES_PER_BATCH * sizeof(CreatureInstanceDataDX12);
    instBufDesc.usage = BufferUsage::Vertex;  // Used as second vertex buffer slot
    instBufDesc.cpuAccess = true;  // Updated every frame via Map/Unmap

    for (uint32_t i = 0; i < NUM_FRAMES_IN_FLIGHT_CREATURE; ++i) {
        char debugName[64];
        snprintf(debugName, sizeof(debugName), "CreatureInstBuf_%d_%d_%d[%u]",
                 static_cast<int>(key.type), key.sizeCategory, key.speedCategory, i);
        instBufDesc.debugName = debugName;

        buffers.instanceBuffer[i] = m_device->CreateBuffer(instBufDesc);
        if (!buffers.instanceBuffer[i]) {
            std::cerr << "[CreatureRendererDX12] Failed to create instance buffer " << i
                      << " for mesh key" << std::endl;
            return false;
        }
    }

    m_instanceBuffers[key] = std::move(buffers);
    return true;
}

MeshDataDX12* CreatureRendererDX12::getOrCreateDX12Mesh(const MeshKey& key, const Genome& genome, CreatureType type) {
    // Check if already cached
    auto it = m_dx12MeshCache.find(key);
    if (it != m_dx12MeshCache.end()) {
        return &it->second;
    }

    // Get OpenGL mesh from the original cache
    MeshData* glMesh = m_meshCache->getMesh(genome, type);
    if (!glMesh || glMesh->vertices.empty() || glMesh->indices.empty()) {
        std::cerr << "[CreatureRendererDX12] Failed to get mesh from cache" << std::endl;
        return nullptr;
    }

    // Convert to DX12 format
    MeshDataDX12 dx12Mesh;

    // Convert vertices to DX12 format (with proper alignment/padding)
    std::vector<CreatureVertexDX12> dx12Vertices;
    dx12Vertices.reserve(glMesh->vertices.size());

    for (const auto& v : glMesh->vertices) {
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
        dx12Vertices.push_back(dxv);
    }

    // Create vertex buffer
    BufferDesc vbDesc;
    vbDesc.size = dx12Vertices.size() * sizeof(CreatureVertexDX12);
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuAccess = false;  // Default heap, uploaded via staging buffer
    vbDesc.debugName = "CreatureMeshVB";

    dx12Mesh.vertexBuffer = m_device->CreateBuffer(vbDesc);
    if (!dx12Mesh.vertexBuffer) {
        std::cerr << "[CreatureRendererDX12] Failed to create vertex buffer" << std::endl;
        return nullptr;
    }

    if (!uploadStaticBuffer(dx12Mesh.vertexBuffer.get(), dx12Vertices.data(), vbDesc.size,
                            ResourceState::VertexBuffer)) {
        std::cerr << "[CreatureRendererDX12] Failed to upload vertex buffer data" << std::endl;
        return nullptr;
    }

    // Create index buffer
    BufferDesc ibDesc;
    ibDesc.size = glMesh->indices.size() * sizeof(uint32_t);
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuAccess = false;  // Default heap, uploaded via staging buffer
    ibDesc.debugName = "CreatureMeshIB";

    dx12Mesh.indexBuffer = m_device->CreateBuffer(ibDesc);
    if (!dx12Mesh.indexBuffer) {
        std::cerr << "[CreatureRendererDX12] Failed to create index buffer" << std::endl;
        return nullptr;
    }

    if (!uploadStaticBuffer(dx12Mesh.indexBuffer.get(), glMesh->indices.data(), ibDesc.size,
                            ResourceState::IndexBuffer)) {
        std::cerr << "[CreatureRendererDX12] Failed to upload index buffer data" << std::endl;
        return nullptr;
    }

    dx12Mesh.vertexCount = static_cast<uint32_t>(dx12Vertices.size());
    dx12Mesh.indexCount = static_cast<uint32_t>(glMesh->indices.size());
    dx12Mesh.vertexStride = sizeof(CreatureVertexDX12);

    // Convert bounds
    dx12Mesh.boundsMin = Vec3(glMesh->boundsMin.x, glMesh->boundsMin.y, glMesh->boundsMin.z);
    dx12Mesh.boundsMax = Vec3(glMesh->boundsMax.x, glMesh->boundsMax.y, glMesh->boundsMax.z);

    // Store in cache
    m_dx12MeshCache[key] = std::move(dx12Mesh);
    return &m_dx12MeshCache[key];
}

bool CreatureRendererDX12::uploadStaticBuffer(IBuffer* dstBuffer, const void* data, size_t size,
                                              ResourceState finalState) {
    if (!dstBuffer || !data || size == 0) {
        return false;
    }

    if (!m_uploadCommandList || !m_uploadFence) {
        return false;
    }

    BufferDesc uploadDesc;
    uploadDesc.size = size;
    uploadDesc.usage = BufferUsage::CopySrc;
    uploadDesc.cpuAccess = true;
    uploadDesc.debugName = "CreatureMeshUpload";

    UniquePtr<IBuffer> uploadBuffer = m_device->CreateBuffer(uploadDesc);
    if (!uploadBuffer) {
        return false;
    }

    void* mapped = uploadBuffer->Map();
    if (!mapped) {
        return false;
    }
    memcpy(mapped, data, size);
    uploadBuffer->Unmap();

    m_uploadCommandList->Begin();
    m_uploadCommandList->ResourceBarrier(dstBuffer, ResourceState::Common, ResourceState::CopyDest);
    m_uploadCommandList->CopyBuffer(uploadBuffer.get(), dstBuffer, 0, 0, size);
    m_uploadCommandList->ResourceBarrier(dstBuffer, ResourceState::CopyDest, finalState);
    m_uploadCommandList->End();

    m_device->Submit(m_uploadCommandList.get());
    m_uploadFenceValue++;
    m_device->SignalFence(m_uploadFence.get(), m_uploadFenceValue);
    m_device->WaitFence(m_uploadFence.get(), m_uploadFenceValue);

    return true;
}

void CreatureRendererDX12::render(
    const std::vector<std::unique_ptr<Creature>>& creatures,
    const Camera& camera,
    ICommandList* cmdList,
    IPipeline* pipeline,
    float time
) {
    // Debug output (once per session)
    static bool firstRenderDebug = true;
    if (firstRenderDebug) {
        int aliveCount = 0;
        int herbCount = 0;
        int carnCount = 0;
        int aquaCount = 0;
        int flyingCount = 0;
        for (const auto& c : creatures) {
            if (c->isAlive()) {
                aliveCount++;
                switch (c->getType()) {
                    case CreatureType::HERBIVORE: herbCount++; break;
                    case CreatureType::CARNIVORE: carnCount++; break;
                    case CreatureType::AQUATIC: aquaCount++; break;
                    case CreatureType::FLYING: flyingCount++; break;
                    default: break;
                }
            }
        }
        std::cout << "[RENDER DX12] First frame: " << aliveCount << " creatures alive ("
                  << herbCount << " herbivores, " << carnCount << " carnivores, "
                  << aquaCount << " aquatic, " << flyingCount << " flying)" << std::endl;
        firstRenderDebug = false;
    }

    if (!m_meshCache || !m_device || creatures.empty()) {
        m_lastDrawCalls = 0;
        m_lastInstancesRendered = 0;
        m_lastCulledCount = 0;
        return;
    }

    // Get the frustum for culling
    const Frustum& frustum = camera.getFrustum();

    // Clear staging buffers
    for (auto& pair : m_stagingBuffers) {
        pair.second.clear();
    }

    // Group creatures by mesh key
    std::unordered_map<MeshKey, RenderBatch> batches;
    int culledCount = 0;

    for (const auto& creature : creatures) {
        if (!creature->isAlive()) continue;

        // Get genome and creature type
        const Genome& genome = creature->getGenome();
        CreatureType type = creature->getType();
        const glm::vec3& position = creature->getPosition();
        float size = genome.size;

        // Frustum culling: use bounding sphere for quick rejection
        // Account for dramatic visual scaling (up to 3.0x)
        // Dramatic size variation: genome size 0.5-2.0 maps to visual 0.3x-3.0x
        // For very large creatures (whales, etc.), scale appropriately
        float visualScale = (size <= 2.0f)
            ? 0.3f + (size - 0.5f) * 1.8f  // Normal creatures: 0.3x to 3.0x
            : 3.0f + (size - 2.0f) * 0.5f; // Large creatures: scale up further
        visualScale = glm::clamp(visualScale, 0.3f, 10.0f);
        float boundingRadius = visualScale * 2.5f;  // Account for dramatic size variation

        if (!frustum.isSphereVisible(position, boundingRadius)) {
            culledCount++;
            continue; // Skip off-screen creatures
        }

        // Get mesh key
        MeshKey key = CreatureMeshCache::getMeshKey(genome, type);

        // Create batch if doesn't exist
        if (batches.find(key) == batches.end()) {
            RenderBatch batch;
            batch.key = key;
            batch.mesh = getOrCreateDX12Mesh(key, genome, type);

            if (!batch.mesh || !batch.mesh->isValid()) {
                continue; // Skip invalid meshes
            }

            // Ensure instance buffer exists for this key
            if (!createInstanceBuffer(key)) {
                continue;
            }

            batches[key] = batch;
        }

        // Create instance data
        CreatureInstanceDataDX12 instanceData;

        // Model matrix (position, rotation, scale)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);

        // Rotation based on movement direction
        glm::vec3 velocity = creature->getVelocity();
        if (glm::length(velocity) > 0.01f) {
            float angle = std::atan2(velocity.x, velocity.z);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Use the visualScale already computed for frustum culling (0.3x to 3.0x)
        model = glm::scale(model, glm::vec3(visualScale));

        float animationPhase = time * glm::length(velocity); // Walk cycle based on speed

        // Use species-tinted color for visual differentiation
        glm::vec3 renderColor = creature->getSpeciesTintedColor();
        instanceData.SetFromCreature(model, renderColor, animationPhase);

        // Add to batch
        batches[key].instances.push_back(instanceData);
    }

    m_lastCulledCount = culledCount;

    // Render each batch
    m_lastDrawCalls = 0;
    m_lastInstancesRendered = 0;

    cmdList->SetPipeline(pipeline);

    for (auto& pair : batches) {
        RenderBatch& batch = pair.second;

        if (batch.instances.empty() || !batch.mesh || !batch.mesh->isValid()) {
            continue;
        }

        // Get instance buffer for this frame
        auto bufIt = m_instanceBuffers.find(batch.key);
        if (bufIt == m_instanceBuffers.end()) {
            continue;
        }

        IBuffer* instBuf = bufIt->second.instanceBuffer[m_frameIndex].get();
        if (!instBuf) {
            continue;
        }

        // Upload instance data via Map/Unmap
        void* instData = instBuf->Map();
        if (instData) {
            size_t dataSize = batch.instances.size() * sizeof(CreatureInstanceDataDX12);
            memcpy(instData, batch.instances.data(), dataSize);
            instBuf->Unmap();
        }

        // Determine creature type for shader
        int typeInt = 0;
        if (batch.key.type == CreatureType::HERBIVORE) typeInt = 0;
        else if (batch.key.type == CreatureType::CARNIVORE) typeInt = 1;
        else if (batch.key.type == CreatureType::AQUATIC) typeInt = 2;
        else if (batch.key.type == CreatureType::FLYING) typeInt = 3;

        renderBatch(batch, cmdList, typeInt);
        m_lastDrawCalls++;
        m_lastInstancesRendered += static_cast<int>(batch.instances.size());
    }
}

void CreatureRendererDX12::renderForShadow(
    const std::vector<std::unique_ptr<Creature>>& creatures,
    ICommandList* cmdList,
    IPipeline* shadowPipeline,
    float time
) {
    if (!m_meshCache || !m_device || creatures.empty()) {
        return;
    }

    // Group creatures by mesh key (same as regular render, but no frustum culling for shadows)
    std::unordered_map<MeshKey, RenderBatch> batches;

    for (const auto& creature : creatures) {
        if (!creature->isAlive()) continue;

        const Genome& genome = creature->getGenome();
        CreatureType type = creature->getType();
        MeshKey key = CreatureMeshCache::getMeshKey(genome, type);

        if (batches.find(key) == batches.end()) {
            RenderBatch batch;
            batch.key = key;
            batch.mesh = getOrCreateDX12Mesh(key, genome, type);

            if (!batch.mesh || !batch.mesh->isValid()) {
                continue;
            }

            if (!createInstanceBuffer(key)) {
                continue;
            }

            batches[key] = batch;
        }

        // Create instance data (only need model matrix for shadow)
        CreatureInstanceDataDX12 instanceData;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, creature->getPosition());

        glm::vec3 velocity = creature->getVelocity();
        if (glm::length(velocity) > 0.01f) {
            float angle = std::atan2(velocity.x, velocity.z);
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Apply same dramatic size scaling as main pass (0.3x to 10.0x)
        float size = genome.size;
        float visualScale = (size <= 2.0f)
            ? 0.3f + (size - 0.5f) * 1.8f  // Normal creatures: 0.3x to 3.0x
            : 3.0f + (size - 2.0f) * 0.5f; // Large creatures: scale up further
        visualScale = glm::clamp(visualScale, 0.3f, 10.0f);
        model = glm::scale(model, glm::vec3(visualScale));

        float animationPhase = time * glm::length(velocity);
        instanceData.SetFromCreature(model, genome.color, animationPhase);

        batches[key].instances.push_back(instanceData);
    }

    // Render each batch for shadow
    cmdList->SetPipeline(shadowPipeline);

    for (auto& pair : batches) {
        RenderBatch& batch = pair.second;

        if (batch.instances.empty() || !batch.mesh || !batch.mesh->isValid()) {
            continue;
        }

        auto bufIt = m_instanceBuffers.find(batch.key);
        if (bufIt == m_instanceBuffers.end()) {
            continue;
        }

        IBuffer* instBuf = bufIt->second.instanceBuffer[m_frameIndex].get();
        if (!instBuf) {
            continue;
        }

        // Upload instance data
        void* instData = instBuf->Map();
        if (instData) {
            size_t dataSize = batch.instances.size() * sizeof(CreatureInstanceDataDX12);
            memcpy(instData, batch.instances.data(), dataSize);
            instBuf->Unmap();
        }

        renderBatch(batch, cmdList, 0); // Creature type doesn't matter for shadow
    }
}

void CreatureRendererDX12::renderBatch(const RenderBatch& batch, ICommandList* cmdList, int creatureType) {
    if (batch.instances.empty() || !batch.mesh || !batch.mesh->isValid()) {
        return;
    }

    // Get instance buffer for this frame
    auto bufIt = m_instanceBuffers.find(batch.key);
    if (bufIt == m_instanceBuffers.end()) {
        return;
    }

    IBuffer* instBuf = bufIt->second.instanceBuffer[m_frameIndex].get();
    if (!instBuf) {
        return;
    }

    // Bind vertex buffers to multiple input slots:
    // Slot 0: Per-vertex mesh data (position, normal, texcoord)
    // Slot 1: Per-instance data (model matrix, color, animation)
    cmdList->BindVertexBuffer(0, batch.mesh->vertexBuffer.get(), batch.mesh->vertexStride, 0);
    cmdList->BindVertexBuffer(1, instBuf, sizeof(CreatureInstanceDataDX12), 0);

    // Bind index buffer
    cmdList->BindIndexBuffer(batch.mesh->indexBuffer.get(), IndexFormat::UInt32, 0);

    // Draw all instances in a single call
    uint32_t instanceCount = static_cast<uint32_t>(batch.instances.size());
    cmdList->DrawIndexedInstanced(batch.mesh->indexCount, instanceCount, 0, 0, 0);
}
