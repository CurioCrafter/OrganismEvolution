#include "TreeRenderer_DX12.h"
#include "../mesh/MeshData.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

TreeRendererDX12::TreeRendererDX12()
    : m_device(nullptr)
    , m_vegManager(nullptr)
    , m_frameIndex(0)
    , m_renderedCount(0)
    , m_totalCount(0)
    , m_culledCount(0)
    , m_drawCallCount(0)
    , m_windDirection(1.0f, 0.0f)
    , m_windStrength(0.2f)
{
}

TreeRendererDX12::~TreeRendererDX12() {
    // UniquePtr handles cleanup automatically
    m_treeMeshes.clear();
    m_instanceBuffers.clear();
    m_stagingInstances.clear();
}

bool TreeRendererDX12::init(IDevice* device, VegetationManager* vegManager) {
    m_device = device;
    m_vegManager = vegManager;

    if (!m_device) {
        std::cerr << "[TreeRendererDX12] ERROR: Null device!" << std::endl;
        return false;
    }

    if (!m_vegManager) {
        std::cerr << "[TreeRendererDX12] ERROR: Null vegetation manager!" << std::endl;
        return false;
    }

    // Create upload command list
    m_uploadCommandList = m_device->CreateCommandList(CommandListType::Graphics);
    if (!m_uploadCommandList) {
        std::cerr << "[TreeRendererDX12] ERROR: Failed to create upload command list!" << std::endl;
        return false;
    }

    // Create upload fence
    m_uploadFence = m_device->CreateFence(0);
    if (!m_uploadFence) {
        std::cerr << "[TreeRendererDX12] ERROR: Failed to create upload fence!" << std::endl;
        return false;
    }

    // Create constant buffer (per-draw offsets)
    BufferDesc cbDesc;
    cbDesc.size = sizeof(TreeConstants) * MAX_TREE_DRAWS;
    cbDesc.usage = BufferUsage::Uniform;
    cbDesc.cpuAccess = true;  // Updated every frame
    cbDesc.debugName = "TreeConstantBuffer";

    m_constantBuffer = m_device->CreateBuffer(cbDesc);
    if (!m_constantBuffer) {
        std::cerr << "[TreeRendererDX12] ERROR: Failed to create constant buffer!" << std::endl;
        return false;
    }

    std::cout << "[TreeRendererDX12] Initialized successfully" << std::endl;
    return true;
}

void TreeRendererDX12::generateTreeMeshes() {
    if (!m_vegManager) {
        std::cerr << "[TreeRendererDX12] Cannot generate meshes - no vegetation manager" << std::endl;
        return;
    }

    std::cout << "[TreeRendererDX12] Generating tree meshes for all types..." << std::endl;

    int meshCount = 0;
    int failCount = 0;

    // Generate mesh for each tree type
    for (int i = 0; i < static_cast<int>(TreeType::COUNT); ++i) {
        TreeType type = static_cast<TreeType>(i);

        if (createMeshForType(type)) {
            meshCount++;

            // Also create instance buffer for this type
            if (!createInstanceBuffer(type)) {
                std::cerr << "[TreeRendererDX12] Failed to create instance buffer for type " << i << std::endl;
            }
        } else {
            failCount++;
        }
    }

    std::cout << "[TreeRendererDX12] Generated " << meshCount << " tree meshes";
    if (failCount > 0) {
        std::cout << " (" << failCount << " failed)";
    }
    std::cout << std::endl;
}

bool TreeRendererDX12::createMeshForType(TreeType type) {
    // Get the mesh from vegetation manager
    const MeshData* srcMesh = m_vegManager->getMeshForType(type);
    if (!srcMesh || srcMesh->vertices.empty() || srcMesh->indices.empty()) {
        // This type may not have a mesh (not all types are generated)
        return false;
    }

    TreeMeshDX12 dx12Mesh;

    // Convert vertices to DX12 format with proper alignment
    std::vector<TreeVertexDX12> dx12Vertices;
    dx12Vertices.reserve(srcMesh->vertices.size());

    for (const auto& v : srcMesh->vertices) {
        TreeVertexDX12 dxv;
        dxv.position[0] = v.position.x;
        dxv.position[1] = v.position.y;
        dxv.position[2] = v.position.z;
        dxv.padding1 = 0.0f;
        dxv.normal[0] = v.normal.x;
        dxv.normal[1] = v.normal.y;
        dxv.normal[2] = v.normal.z;
        dxv.padding2 = 0.0f;
        // texCoord stores color data from tree generator
        dxv.texCoord[0] = v.texCoord.x;  // Color R
        dxv.texCoord[1] = v.texCoord.y;  // Color G
        dx12Vertices.push_back(dxv);
    }

    // Create vertex buffer (GPU-only, uploaded via staging)
    BufferDesc vbDesc;
    vbDesc.size = dx12Vertices.size() * sizeof(TreeVertexDX12);
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuAccess = false;
    vbDesc.debugName = "TreeMeshVB";

    dx12Mesh.vertexBuffer = m_device->CreateBuffer(vbDesc);
    if (!dx12Mesh.vertexBuffer) {
        std::cerr << "[TreeRendererDX12] Failed to create vertex buffer for type "
                  << static_cast<int>(type) << std::endl;
        return false;
    }

    // Upload vertex data
    if (!uploadStaticBuffer(dx12Mesh.vertexBuffer.get(), dx12Vertices.data(), vbDesc.size,
                            ResourceState::VertexBuffer)) {
        std::cerr << "[TreeRendererDX12] Failed to upload vertex buffer for type "
                  << static_cast<int>(type) << std::endl;
        return false;
    }

    // Create index buffer
    BufferDesc ibDesc;
    ibDesc.size = srcMesh->indices.size() * sizeof(uint32_t);
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuAccess = false;
    ibDesc.debugName = "TreeMeshIB";

    dx12Mesh.indexBuffer = m_device->CreateBuffer(ibDesc);
    if (!dx12Mesh.indexBuffer) {
        std::cerr << "[TreeRendererDX12] Failed to create index buffer for type "
                  << static_cast<int>(type) << std::endl;
        return false;
    }

    // Upload index data
    if (!uploadStaticBuffer(dx12Mesh.indexBuffer.get(), srcMesh->indices.data(), ibDesc.size,
                            ResourceState::IndexBuffer)) {
        std::cerr << "[TreeRendererDX12] Failed to upload index buffer for type "
                  << static_cast<int>(type) << std::endl;
        return false;
    }

    dx12Mesh.vertexCount = static_cast<uint32_t>(dx12Vertices.size());
    dx12Mesh.indexCount = static_cast<uint32_t>(srcMesh->indices.size());
    dx12Mesh.vertexStride = sizeof(TreeVertexDX12);

    // Convert bounds
    dx12Mesh.boundsMin = Vec3(srcMesh->boundsMin.x, srcMesh->boundsMin.y, srcMesh->boundsMin.z);
    dx12Mesh.boundsMax = Vec3(srcMesh->boundsMax.x, srcMesh->boundsMax.y, srcMesh->boundsMax.z);

    // Store in cache
    m_treeMeshes[type] = std::move(dx12Mesh);
    return true;
}

bool TreeRendererDX12::createInstanceBuffer(TreeType type) {
    if (m_instanceBuffers.find(type) != m_instanceBuffers.end()) {
        return true;  // Already exists
    }

    TypeInstanceBuffers buffers;

    BufferDesc instBufDesc;
    instBufDesc.size = MAX_TREES_PER_TYPE * sizeof(TreeInstanceGPU);
    instBufDesc.usage = BufferUsage::Vertex;  // Used as second vertex buffer slot
    instBufDesc.cpuAccess = true;  // Updated every frame via Map/Unmap

    for (uint32_t i = 0; i < NUM_FRAMES_IN_FLIGHT_TREE; ++i) {
        char debugName[64];
        snprintf(debugName, sizeof(debugName), "TreeInstBuf_%d[%u]", static_cast<int>(type), i);
        instBufDesc.debugName = debugName;

        buffers.instanceBuffer[i] = m_device->CreateBuffer(instBufDesc);
        if (!buffers.instanceBuffer[i]) {
            std::cerr << "[TreeRendererDX12] Failed to create instance buffer " << i
                      << " for tree type " << static_cast<int>(type) << std::endl;
            return false;
        }
    }

    m_instanceBuffers[type] = std::move(buffers);
    return true;
}

bool TreeRendererDX12::uploadStaticBuffer(IBuffer* dstBuffer, const void* data, size_t size,
                                          ResourceState finalState) {
    if (!dstBuffer || !data || size == 0) {
        return false;
    }

    if (!m_uploadCommandList || !m_uploadFence) {
        return false;
    }

    // Create staging buffer
    BufferDesc uploadDesc;
    uploadDesc.size = size;
    uploadDesc.usage = BufferUsage::CopySrc;
    uploadDesc.cpuAccess = true;
    uploadDesc.debugName = "TreeMeshUpload";

    UniquePtr<IBuffer> uploadBuffer = m_device->CreateBuffer(uploadDesc);
    if (!uploadBuffer) {
        return false;
    }

    // Map and copy data to staging buffer
    void* mapped = uploadBuffer->Map();
    if (!mapped) {
        return false;
    }
    memcpy(mapped, data, size);
    uploadBuffer->Unmap();

    // Record copy commands
    m_uploadCommandList->Begin();
    m_uploadCommandList->ResourceBarrier(dstBuffer, ResourceState::Common, ResourceState::CopyDest);
    m_uploadCommandList->CopyBuffer(uploadBuffer.get(), dstBuffer, 0, 0, size);
    m_uploadCommandList->ResourceBarrier(dstBuffer, ResourceState::CopyDest, finalState);
    m_uploadCommandList->End();

    // Execute and wait
    m_device->Submit(m_uploadCommandList.get());
    m_uploadFenceValue++;
    m_device->SignalFence(m_uploadFence.get(), m_uploadFenceValue);
    m_device->WaitFence(m_uploadFence.get(), m_uploadFenceValue);

    return true;
}

void TreeRendererDX12::collectVisibleInstances(const Frustum& frustum, const glm::vec3& cameraPos) {
    // Clear previous frame's staging data
    for (auto& pair : m_stagingInstances) {
        pair.second.clear();
    }

    // Clear LOD batches
    for (int i = 0; i < 6; ++i) {
        for (auto& pair : m_lodBatches[i]) {
            pair.second.clear();
        }
    }

    // Reset LOD stats
    m_lodStats.reset();

    if (!m_vegManager) {
        return;
    }

    m_lastCameraPos = cameraPos;
    const auto& treeInstances = m_vegManager->getTreeInstances();
    m_totalCount = static_cast<int>(treeInstances.size());
    m_culledCount = 0;
    m_renderedCount = 0;

    for (const auto& tree : treeInstances) {
        // Check if we have a mesh for this type
        auto meshIt = m_treeMeshes.find(tree.type);
        if (meshIt == m_treeMeshes.end() || !meshIt->second.isValid()) {
            continue;
        }

        // Calculate distance to camera
        float distance = glm::length(tree.position - cameraPos);

        // Determine LOD level
        LOD::TreeLOD lod = LOD::calculateTreeLOD(distance, m_lodConfig);

        // Skip culled trees (beyond max render distance)
        if (lod == LOD::TreeLOD::CULLED) {
            m_culledCount++;
            m_lodStats.treesCulled++;
            continue;
        }

        // Frustum culling with bounding sphere
        float maxScale = std::max({tree.scale.x, tree.scale.y, tree.scale.z});
        const TreeMeshDX12& mesh = meshIt->second;

        // Calculate bounding sphere radius from mesh bounds
        glm::vec3 boundsSize(
            mesh.boundsMax.x - mesh.boundsMin.x,
            mesh.boundsMax.y - mesh.boundsMin.y,
            mesh.boundsMax.z - mesh.boundsMin.z
        );
        glm::vec3 boundsCenter(
            (mesh.boundsMin.x + mesh.boundsMax.x) * 0.5f,
            (mesh.boundsMin.y + mesh.boundsMax.y) * 0.5f,
            (mesh.boundsMin.z + mesh.boundsMax.z) * 0.5f
        );
        float meshRadius = glm::length(boundsSize) * 0.5f;
        float boundingRadius = meshRadius * maxScale * 1.1f;
        glm::vec3 sphereCenter = tree.position + boundsCenter * tree.scale;

        if (!frustum.isSphereVisible(sphereCenter, boundingRadius)) {
            m_culledCount++;
            continue;
        }

        // Create GPU instance data with LOD information
        TreeInstanceGPU gpuInstance;
        gpuInstance.SetFromInstance(tree);
        gpuInstance.distance = distance;
        gpuInstance.lodLevel = static_cast<uint32_t>(lod);
        gpuInstance.fadeFactor = LOD::calculateTreeFade(distance, lod, m_lodConfig);

        // Add to staging buffer for this tree type
        auto& staging = m_stagingInstances[tree.type];
        if (staging.size() < MAX_TREES_PER_TYPE) {
            staging.push_back(gpuInstance);
            m_renderedCount++;

            // Update LOD stats
            switch (lod) {
                case LOD::TreeLOD::FULL_MESH: m_lodStats.treesFullMesh++; break;
                case LOD::TreeLOD::SIMPLIFIED: m_lodStats.treesSimplified++; break;
                case LOD::TreeLOD::BILLBOARD: m_lodStats.treesBillboard++; break;
                case LOD::TreeLOD::IMPOSTOR: m_lodStats.treesImpostor++; break;
                case LOD::TreeLOD::POINT: m_lodStats.treesPoint++; break;
                default: break;
            }
        } else {
            m_culledCount++;
        }
    }

    // Sort instances by distance for better rendering (front-to-back for opaque)
    sortInstancesByDistance();
}

void TreeRendererDX12::sortInstancesByDistance() {
    for (auto& pair : m_stagingInstances) {
        auto& instances = pair.second;
        std::sort(instances.begin(), instances.end(),
            [](const TreeInstanceGPU& a, const TreeInstanceGPU& b) {
                return a.distance < b.distance;
            });
    }
}

void TreeRendererDX12::updateLODStats() {
    m_lodStats.treeDrawCalls = m_drawCallCount;
}

int TreeRendererDX12::getTreeCountAtLOD(LOD::TreeLOD lod) const {
    switch (lod) {
        case LOD::TreeLOD::FULL_MESH: return m_lodStats.treesFullMesh;
        case LOD::TreeLOD::SIMPLIFIED: return m_lodStats.treesSimplified;
        case LOD::TreeLOD::BILLBOARD: return m_lodStats.treesBillboard;
        case LOD::TreeLOD::IMPOSTOR: return m_lodStats.treesImpostor;
        case LOD::TreeLOD::POINT: return m_lodStats.treesPoint;
        case LOD::TreeLOD::CULLED: return m_lodStats.treesCulled;
        default: return 0;
    }
}

glm::mat4 TreeRendererDX12::buildModelMatrix(const TreeInstanceGPU& instance) const {
    glm::mat4 model = glm::mat4(1.0f);

    // Translate to position
    model = glm::translate(model, glm::vec3(instance.position[0], instance.position[1], instance.position[2]));

    // Rotate around Y axis
    model = glm::rotate(model, instance.rotation, glm::vec3(0.0f, 1.0f, 0.0f));

    // Apply scale
    model = glm::scale(model, glm::vec3(instance.scale[0], instance.scale[1], instance.scale[2]));

    return model;
}

void TreeRendererDX12::render(
    ICommandList* cmdList,
    IPipeline* pipeline,
    const glm::mat4& viewProj,
    const glm::vec3& cameraPos,
    const glm::vec3& lightDir,
    const glm::vec3& lightColor,
    float time
) {
    if (!m_device || !m_vegManager || !cmdList || !pipeline) {
        m_renderedCount = 0;
        m_drawCallCount = 0;
        return;
    }

    // Build frustum and collect visible instances with culling and LOD
    Frustum frustum;
    frustum.update(viewProj);
    collectVisibleInstances(frustum, cameraPos);

    if (m_renderedCount == 0) {
        m_drawCallCount = 0;
        return;
    }

    // Set pipeline once for all batches
    cmdList->SetPipeline(pipeline);

    m_drawCallCount = 0;
    bool drawLimitReached = false;
    void* cbBase = m_constantBuffer ? m_constantBuffer->Map() : nullptr;
    if (!cbBase) {
        std::cerr << "[TreeRendererDX12] ERROR: Failed to map tree constant buffer!" << std::endl;
        return;
    }
    auto* cbBytes = static_cast<uint8_t*>(cbBase);
    size_t drawIndex = 0;

    // Render each tree type as a batch
    for (auto& pair : m_stagingInstances) {
        TreeType type = pair.first;
        std::vector<TreeInstanceGPU>& instances = pair.second;

        if (instances.empty()) {
            continue;
        }

        // Get mesh for this type
        auto meshIt = m_treeMeshes.find(type);
        if (meshIt == m_treeMeshes.end() || !meshIt->second.isValid()) {
            continue;
        }
        TreeMeshDX12& mesh = meshIt->second;

        // Get instance buffer for this type
        auto bufIt = m_instanceBuffers.find(type);
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
            size_t dataSize = instances.size() * sizeof(TreeInstanceGPU);
            memcpy(instData, instances.data(), dataSize);
            instBuf->Unmap();
        }

        // Render each instance with LOD-based fade and fog
        for (size_t i = 0; i < instances.size(); ++i) {
            const TreeInstanceGPU& inst = instances[i];
            if (drawIndex >= MAX_TREE_DRAWS) {
                drawLimitReached = true;
                break;
            }

            // Build model matrix for this tree
            glm::mat4 model = buildModelMatrix(inst);

            // Update constant buffer
            TreeConstants* constants = reinterpret_cast<TreeConstants*>(
                cbBytes + drawIndex * sizeof(TreeConstants));

            // Match other renderers: upload column-major matrices directly.
            memcpy(constants->viewProj, glm::value_ptr(viewProj), sizeof(float) * 16);
            memcpy(constants->model, glm::value_ptr(model), sizeof(float) * 16);

            // Camera position
            constants->cameraPos[0] = cameraPos.x;
            constants->cameraPos[1] = cameraPos.y;
            constants->cameraPos[2] = cameraPos.z;
            constants->cameraPos[3] = 1.0f;

            // Light direction
            constants->lightDir[0] = lightDir.x;
            constants->lightDir[1] = lightDir.y;
            constants->lightDir[2] = lightDir.z;
            constants->lightDir[3] = 0.0f;

            // Light color
            constants->lightColor[0] = lightColor.x;
            constants->lightColor[1] = lightColor.y;
            constants->lightColor[2] = lightColor.z;
            constants->lightColor[3] = 1.0f;

            // Wind parameters
            constants->windParams[0] = m_windDirection.x;
            constants->windParams[1] = m_windDirection.y;
            constants->windParams[2] = m_windStrength;
            constants->windParams[3] = time;

            // Fog parameters for smooth LOD transitions
            constants->fogParams[0] = m_lodConfig.fogStart;
            constants->fogParams[1] = m_lodConfig.fogEnd;
            constants->fogParams[2] = m_lodConfig.fogDensity;
            constants->fogParams[3] = inst.fadeFactor;  // LOD fade factor

            // Fog color
            constants->fogColor[0] = m_lodConfig.fogColor.x;
            constants->fogColor[1] = m_lodConfig.fogColor.y;
            constants->fogColor[2] = m_lodConfig.fogColor.z;
            constants->fogColor[3] = 0.0f;

            // Zero padding
            memset(constants->padding, 0, sizeof(constants->padding));

            // Bind constant buffer
            cmdList->BindConstantBuffer(0, m_constantBuffer.get(),
                                        static_cast<uint32_t>(drawIndex * sizeof(TreeConstants)));

            // Bind vertex buffer (mesh vertices)
            cmdList->BindVertexBuffer(0, mesh.vertexBuffer.get(), mesh.vertexStride, 0);

            // Bind index buffer
            cmdList->BindIndexBuffer(mesh.indexBuffer.get(), IndexFormat::UInt32, 0);

            // Draw
            cmdList->DrawIndexed(mesh.indexCount, 0, 0);
            m_drawCallCount++;
            drawIndex++;
        }
        if (drawLimitReached) {
            break;
        }
    }

    m_constantBuffer->Unmap();
    if (drawLimitReached) {
        static bool warnedOnce = false;
        if (!warnedOnce) {
            std::cerr << "[TreeRendererDX12] WARNING: Tree draw limit reached ("
                      << MAX_TREE_DRAWS << "). Increase MAX_TREE_DRAWS if needed." << std::endl;
            warnedOnce = true;
        }
    }

    // Update LOD stats
    updateLODStats();
}

void TreeRendererDX12::renderForShadow(
    ICommandList* cmdList,
    IPipeline* shadowPipeline,
    const glm::mat4& lightViewProj
) {
    if (!m_device || !m_vegManager || !cmdList || !shadowPipeline) {
        return;
    }

    // For shadow pass, render all trees without frustum culling
    // (shadow map needs all casters)
    const auto& treeInstances = m_vegManager->getTreeInstances();

    if (treeInstances.empty()) {
        return;
    }

    cmdList->SetPipeline(shadowPipeline);

    // Group by type and render
    std::unordered_map<TreeType, std::vector<TreeInstanceGPU>> shadowInstances;

    for (const auto& tree : treeInstances) {
        auto meshIt = m_treeMeshes.find(tree.type);
        if (meshIt == m_treeMeshes.end() || !meshIt->second.isValid()) {
            continue;
        }

        TreeInstanceGPU gpuInstance;
        gpuInstance.SetFromInstance(tree);
        shadowInstances[tree.type].push_back(gpuInstance);
    }

    // Render each type
    bool drawLimitReached = false;
    void* cbBase = m_constantBuffer ? m_constantBuffer->Map() : nullptr;
    if (!cbBase) {
        std::cerr << "[TreeRendererDX12] ERROR: Failed to map tree constant buffer (shadow)!" << std::endl;
        return;
    }
    auto* cbBytes = static_cast<uint8_t*>(cbBase);
    size_t drawIndex = 0;

    for (auto& pair : shadowInstances) {
        TreeType type = pair.first;
        std::vector<TreeInstanceGPU>& instances = pair.second;

        if (instances.empty()) {
            continue;
        }

        auto meshIt = m_treeMeshes.find(type);
        if (meshIt == m_treeMeshes.end()) {
            continue;
        }
        TreeMeshDX12& mesh = meshIt->second;

        for (const auto& inst : instances) {
            if (drawIndex >= MAX_TREE_DRAWS) {
                drawLimitReached = true;
                break;
            }
            glm::mat4 model = buildModelMatrix(inst);
            glm::mat4 mvp = lightViewProj * model;

            // Update constant buffer with light MVP
            TreeConstants* constants = reinterpret_cast<TreeConstants*>(
                cbBytes + drawIndex * sizeof(TreeConstants));
            memset(constants, 0, sizeof(TreeConstants));

            // Use viewProj slot for light MVP in shadow pass
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    constants->viewProj[row * 4 + col] = mvp[col][row];
                }
            }

            // Identity model matrix (MVP already includes it)
            glm::mat4 identity(1.0f);
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    constants->model[row * 4 + col] = identity[col][row];
                }
            }

            cmdList->BindConstantBuffer(0, m_constantBuffer.get(),
                                        static_cast<uint32_t>(drawIndex * sizeof(TreeConstants)));
            cmdList->BindVertexBuffer(0, mesh.vertexBuffer.get(), mesh.vertexStride, 0);
            cmdList->BindIndexBuffer(mesh.indexBuffer.get(), IndexFormat::UInt32, 0);
            cmdList->DrawIndexed(mesh.indexCount, 0, 0);
            drawIndex++;
        }
        if (drawLimitReached) {
            break;
        }
    }

    m_constantBuffer->Unmap();
    if (drawLimitReached) {
        static bool warnedOnce = false;
        if (!warnedOnce) {
            std::cerr << "[TreeRendererDX12] WARNING: Shadow tree draw limit reached ("
                      << MAX_TREE_DRAWS << "). Increase MAX_TREE_DRAWS if needed." << std::endl;
            warnedOnce = true;
        }
    }
}

void TreeRendererDX12::renderBatch(const RenderBatch& batch, ICommandList* cmdList) {
    // This is used for batched instanced rendering (future optimization)
    // Currently not used as we render per-tree with constant buffer updates
    (void)batch;
    (void)cmdList;
}
