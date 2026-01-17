#include "SmallCreatureRenderer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace small {

// =============================================================================
// SmallCreatureRenderer Implementation
// =============================================================================

SmallCreatureRenderer::SmallCreatureRenderer()
    : device_(nullptr) {
    instances_.reserve(MAX_INSTANCES);
    pointSprites_.reserve(MAX_POINT_SPRITES);
    particles_.reserve(MAX_PARTICLES);
    trails_.reserve(MAX_TRAIL_SEGMENTS);
}

SmallCreatureRenderer::~SmallCreatureRenderer() = default;

bool SmallCreatureRenderer::initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) {
    device_ = device;

    createShaders(device);
    createCreatureMeshes(device, cmdList);
    createInstanceBuffers(device);

    return true;
}

void SmallCreatureRenderer::prepareFrame(const XMFLOAT3& cameraPos, const XMFLOAT4X4& viewProj) {
    cameraPos_ = cameraPos;
    viewProj_ = viewProj;

    // Clear previous frame data
    instances_.clear();
    pointSprites_.clear();
    particles_.clear();
    batches_.clear();

    totalInstances_ = 0;
    detailedCount_ = 0;
    simplifiedCount_ = 0;
    pointCount_ = 0;
    particleCount_ = 0;
}

void SmallCreatureRenderer::buildRenderData(SmallCreatureManager& manager) {
    const auto& creatures = manager.getCreatures();

    // Reset habitat stats
    habitatStats_ = HabitatStats{};

    for (const auto& creature : creatures) {
        if (!creature.isAlive()) continue;

        // Track habitat distribution
        auto props = getProperties(creature.type);
        switch (props.primaryHabitat) {
            case HabitatType::GROUND_SURFACE:
                habitatStats_.groundCount++;
                break;
            case HabitatType::GRASS:
                habitatStats_.grassCount++;
                break;
            case HabitatType::BUSH:
                habitatStats_.bushCount++;
                break;
            case HabitatType::AERIAL:
                habitatStats_.aerialCount++;
                break;
            case HabitatType::CANOPY:
            case HabitatType::TREE_TRUNK:
                habitatStats_.canopyCount++;
                break;
            case HabitatType::UNDERGROUND:
                habitatStats_.undergroundCount++;
                break;
            case HabitatType::WATER_SURFACE:
            case HabitatType::UNDERWATER:
                habitatStats_.aquaticCount++;
                break;
        }

        // Track size category distribution
        switch (props.sizeCategory) {
            case SizeCategory::MICROSCOPIC:
                habitatStats_.microscopicCount++;
                break;
            case SizeCategory::TINY:
                habitatStats_.tinyCount++;
                break;
            case SizeCategory::SMALL:
                habitatStats_.smallCount++;
                break;
            case SizeCategory::MEDIUM:
                habitatStats_.mediumCount++;
                break;
        }

        // Calculate distance to camera
        float dx = creature.position.x - cameraPos_.x;
        float dy = creature.position.y - cameraPos_.y;
        float dz = creature.position.z - cameraPos_.z;
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);

        if (distance > maxRenderDistance_) continue;

        LODLevel lod = calculateLOD(distance);

        if (lod == LODLevel::PARTICLE) {
            // Very distant - aggregate into particles
            // Handled separately in swarm visualizer
            particleCount_++;
            continue;
        }

        if (lod == LODLevel::POINT) {
            // Point sprite
            PointSpriteInstance sprite;
            sprite.position = creature.position;

            auto props = getProperties(creature.type);
            // Calculate proper world-scale size in meters, then apply visibility bias
            // based on size category for readability at distance
            float worldSize = props.minSize + (props.maxSize - props.minSize) * creature.genome->size;
            float visibilityBias = getVisibilityBias(props.sizeCategory);
            sprite.size = worldSize * visibilityBias;

            sprite.color = {
                creature.genome->colorR,
                creature.genome->colorG,
                creature.genome->colorB,
                1.0f
            };

            pointSprites_.push_back(sprite);
            pointCount_++;
            continue;
        }

        // Detailed or simplified mesh instance
        SmallCreatureInstance instance;

        // Build world matrix
        auto props = getProperties(creature.type);
        // Calculate world-scale size in meters: interpolate between min and max based on genome
        float worldSize = props.minSize + (props.maxSize - props.minSize) * creature.genome->size;
        // Apply mesh-to-world scale factor (meshes are authored at ~1 unit scale)
        float scale = worldSize * getMeshScaleFactor(creature.type);

        // Scale matrix
        XMMATRIX scaleM = XMMatrixScaling(scale, scale, scale);

        // Rotation matrix (Y-axis)
        XMMATRIX rotM = XMMatrixRotationY(creature.rotation);

        // Translation matrix
        XMMATRIX transM = XMMatrixTranslation(creature.position.x,
                                               creature.position.y,
                                               creature.position.z);

        // Combine: Scale -> Rotate -> Translate
        XMMATRIX worldM = scaleM * rotM * transM;
        XMStoreFloat4x4(&instance.world, XMMatrixTranspose(worldM));

        instance.color = {
            creature.genome->colorR,
            creature.genome->colorG,
            creature.genome->colorB,
            1.0f
        };

        instance.params = {
            scale,
            creature.animationTime,
            static_cast<float>(creature.type),
            static_cast<float>(lod)
        };

        instances_.push_back(instance);

        if (lod == LODLevel::DETAILED) detailedCount_++;
        else simplifiedCount_++;
    }

    totalInstances_ = instances_.size() + pointCount_ + particleCount_;

    sortAndBatch();
}

void SmallCreatureRenderer::render(ID3D12GraphicsCommandList* cmdList) {
    if (instances_.empty() && pointSprites_.empty()) return;

    uploadInstanceData(cmdList);

    // Set root signature
    cmdList->SetGraphicsRootSignature(rootSignature_.Get());

    // Set view-projection constant
    // cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewProj_, 0);

    // Render instanced meshes
    if (!instances_.empty() && instancedPSO_) {
        cmdList->SetPipelineState(instancedPSO_.Get());

        for (const auto& batch : batches_) {
            if (batch.lod == LODLevel::POINT || batch.lod == LODLevel::PARTICLE) continue;

            CreatureMesh* mesh = getMesh(batch.type, batch.lod);
            if (!mesh) continue;

            cmdList->IASetVertexBuffers(0, 1, &mesh->vbView);
            cmdList->IASetIndexBuffer(&mesh->ibView);
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Instance buffer as second vertex stream
            D3D12_VERTEX_BUFFER_VIEW instanceView = {};
            instanceView.BufferLocation = instanceBuffer_->GetGPUVirtualAddress() +
                                          batch.startIndex * sizeof(SmallCreatureInstance);
            instanceView.SizeInBytes = batch.instanceCount * sizeof(SmallCreatureInstance);
            instanceView.StrideInBytes = sizeof(SmallCreatureInstance);
            cmdList->IASetVertexBuffers(1, 1, &instanceView);

            cmdList->DrawIndexedInstanced(mesh->indexCount, batch.instanceCount, 0, 0, 0);
        }
    }

    // Render point sprites
    if (!pointSprites_.empty() && pointSpritePSO_) {
        cmdList->SetPipelineState(pointSpritePSO_.Get());

        D3D12_VERTEX_BUFFER_VIEW spriteView = {};
        spriteView.BufferLocation = pointSpriteBuffer_->GetGPUVirtualAddress();
        spriteView.SizeInBytes = static_cast<UINT>(pointSprites_.size() * sizeof(PointSpriteInstance));
        spriteView.StrideInBytes = sizeof(PointSpriteInstance);
        cmdList->IASetVertexBuffers(0, 1, &spriteView);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
        cmdList->DrawInstanced(static_cast<UINT>(pointSprites_.size()), 1, 0, 0);
    }
}

void SmallCreatureRenderer::renderPheromoneTrails(ID3D12GraphicsCommandList* cmdList,
                                                   const std::vector<PheromonePoint>& pheromones) {
    if (!showPheromones_ || pheromones.empty()) return;

    trails_.clear();

    // Build trail segments from pheromone points
    for (size_t i = 0; i < pheromones.size() && trails_.size() < MAX_TRAIL_SEGMENTS; ++i) {
        const auto& p1 = pheromones[i];
        if (p1.strength < 0.1f) continue;

        // Find nearest same-colony pheromone to connect
        for (size_t j = i + 1; j < pheromones.size(); ++j) {
            const auto& p2 = pheromones[j];
            if (p2.colonyID != p1.colonyID || p2.type != p1.type) continue;

            float dx = p2.position.x - p1.position.x;
            float dy = p2.position.y - p1.position.y;
            float dz = p2.position.z - p1.position.z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);

            if (dist < 0.5f) {  // Connect nearby points
                TrailSegment seg;
                seg.start = p1.position;
                seg.end = p2.position;
                seg.strength = (p1.strength + p2.strength) * 0.5f;

                // Color by type
                switch (p1.type) {
                    case PheromonePoint::Type::FOOD_TRAIL:
                        seg.color = { 0.0f, 1.0f, 0.0f, seg.strength };
                        break;
                    case PheromonePoint::Type::HOME_TRAIL:
                        seg.color = { 0.0f, 0.0f, 1.0f, seg.strength };
                        break;
                    case PheromonePoint::Type::ALARM:
                        seg.color = { 1.0f, 0.0f, 0.0f, seg.strength };
                        break;
                    default:
                        seg.color = { 1.0f, 1.0f, 0.0f, seg.strength };
                }

                trails_.push_back(seg);
                break;
            }
        }
    }

    if (trails_.empty()) return;

    // Upload and render trails
    // ... (trail rendering implementation)
}

void SmallCreatureRenderer::createCreatureMeshes(ID3D12Device* device,
                                                  ID3D12GraphicsCommandList* cmdList) {
    // Create meshes for common types
    // Insects
    createInsectMesh(device, SmallCreatureType::ANT_WORKER);
    createInsectMesh(device, SmallCreatureType::BEETLE_GROUND);
    createInsectMesh(device, SmallCreatureType::BUTTERFLY);
    createInsectMesh(device, SmallCreatureType::BEE_WORKER);
    createInsectMesh(device, SmallCreatureType::DRAGONFLY);

    // Arachnids
    createArachnidMesh(device, SmallCreatureType::SPIDER_ORB_WEAVER);
    createArachnidMesh(device, SmallCreatureType::SCORPION);

    // Small mammals
    createSmallMammalMesh(device, SmallCreatureType::MOUSE);
    createSmallMammalMesh(device, SmallCreatureType::SQUIRREL_TREE);
    createSmallMammalMesh(device, SmallCreatureType::RABBIT);

    // Reptiles
    createReptileMesh(device, SmallCreatureType::LIZARD_SMALL);
    createReptileMesh(device, SmallCreatureType::SNAKE_SMALL);

    // Amphibians
    createAmphibianMesh(device, SmallCreatureType::FROG);
}

void SmallCreatureRenderer::createInsectMesh(ID3D12Device* device, SmallCreatureType type) {
    std::vector<ProceduralMeshGenerator::Vertex> vertices;
    std::vector<uint16_t> indices;

    // Generate based on type
    if (type == SmallCreatureType::ANT_WORKER || isAnt(type)) {
        ProceduralMeshGenerator::generateInsectBody(vertices, indices, 0.2f, 0.3f, 0.4f);
    }
    else if (type == SmallCreatureType::BEETLE_GROUND) {
        ProceduralMeshGenerator::generateInsectBody(vertices, indices, 0.2f, 0.35f, 0.35f);
    }
    else if (type == SmallCreatureType::BUTTERFLY || type == SmallCreatureType::MOTH) {
        // Butterfly with wings
        ProceduralMeshGenerator::generateInsectBody(vertices, indices, 0.15f, 0.2f, 0.3f);
        // Would add wing geometry here
    }
    else if (isBee(type)) {
        ProceduralMeshGenerator::generateInsectBody(vertices, indices, 0.2f, 0.35f, 0.35f);
    }
    else if (type == SmallCreatureType::DRAGONFLY) {
        ProceduralMeshGenerator::generateInsectBody(vertices, indices, 0.15f, 0.2f, 0.5f);
    }
    else {
        // Default insect
        ProceduralMeshGenerator::generateInsectBody(vertices, indices, 0.2f, 0.3f, 0.3f);
    }

    // Create GPU resources
    if (vertices.empty()) return;

    CreatureMesh mesh;
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    // Create vertex buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = vertices.size() * sizeof(ProceduralMeshGenerator::Vertex);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.vertexBuffer));

    mesh.vbView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vbView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.vbView.StrideInBytes = sizeof(ProceduralMeshGenerator::Vertex);

    // Create index buffer
    bufferDesc.Width = indices.size() * sizeof(uint16_t);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.indexBuffer));

    mesh.ibView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.ibView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.ibView.Format = DXGI_FORMAT_R16_UINT;

    meshes_[static_cast<int>(type)] = std::move(mesh);
}

void SmallCreatureRenderer::createArachnidMesh(ID3D12Device* device, SmallCreatureType type) {
    std::vector<ProceduralMeshGenerator::Vertex> vertices;
    std::vector<uint16_t> indices;

    if (isSpider(type)) {
        ProceduralMeshGenerator::generateSpiderBody(vertices, indices, 0.3f);
    }
    else if (type == SmallCreatureType::SCORPION) {
        // Scorpion with tail
        ProceduralMeshGenerator::generateSpiderBody(vertices, indices, 0.35f);
        // Would add tail segments here
    }

    if (vertices.empty()) return;

    // Create GPU resources (same pattern as insects)
    CreatureMesh mesh;
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = vertices.size() * sizeof(ProceduralMeshGenerator::Vertex);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.vertexBuffer));

    mesh.vbView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vbView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.vbView.StrideInBytes = sizeof(ProceduralMeshGenerator::Vertex);

    bufferDesc.Width = indices.size() * sizeof(uint16_t);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.indexBuffer));

    mesh.ibView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.ibView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.ibView.Format = DXGI_FORMAT_R16_UINT;

    meshes_[static_cast<int>(type)] = std::move(mesh);
}

void SmallCreatureRenderer::createSmallMammalMesh(ID3D12Device* device, SmallCreatureType type) {
    std::vector<ProceduralMeshGenerator::Vertex> vertices;
    std::vector<uint16_t> indices;

    if (type == SmallCreatureType::MOUSE || type == SmallCreatureType::RAT) {
        ProceduralMeshGenerator::generateMammalBody(vertices, indices, 0.4f, 0.2f);
    }
    else if (type == SmallCreatureType::SQUIRREL_TREE) {
        ProceduralMeshGenerator::generateMammalBody(vertices, indices, 0.5f, 0.25f);
    }
    else if (type == SmallCreatureType::RABBIT) {
        ProceduralMeshGenerator::generateMammalBody(vertices, indices, 0.6f, 0.35f);
    }

    if (vertices.empty()) return;

    // Create mesh resources...
    CreatureMesh mesh;
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = vertices.size() * sizeof(ProceduralMeshGenerator::Vertex);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.vertexBuffer));

    mesh.vbView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vbView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.vbView.StrideInBytes = sizeof(ProceduralMeshGenerator::Vertex);

    bufferDesc.Width = indices.size() * sizeof(uint16_t);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.indexBuffer));

    mesh.ibView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.ibView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.ibView.Format = DXGI_FORMAT_R16_UINT;

    meshes_[static_cast<int>(type)] = std::move(mesh);
}

void SmallCreatureRenderer::createReptileMesh(ID3D12Device* device, SmallCreatureType type) {
    // Similar pattern - generate mesh and create resources
    // For brevity, using simple quadruped for reptiles
    std::vector<ProceduralMeshGenerator::Vertex> vertices;
    std::vector<uint16_t> indices;

    ProceduralMeshGenerator::generateSimplifiedQuadruped(vertices, indices);

    if (vertices.empty()) return;

    CreatureMesh mesh;
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = vertices.size() * sizeof(ProceduralMeshGenerator::Vertex);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.vertexBuffer));

    mesh.vbView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vbView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.vbView.StrideInBytes = sizeof(ProceduralMeshGenerator::Vertex);

    bufferDesc.Width = indices.size() * sizeof(uint16_t);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.indexBuffer));

    mesh.ibView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.ibView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.ibView.Format = DXGI_FORMAT_R16_UINT;

    meshes_[static_cast<int>(type)] = std::move(mesh);
}

void SmallCreatureRenderer::createAmphibianMesh(ID3D12Device* device, SmallCreatureType type) {
    std::vector<ProceduralMeshGenerator::Vertex> vertices;
    std::vector<uint16_t> indices;

    ProceduralMeshGenerator::generateFrogBody(vertices, indices, 0.3f);

    if (vertices.empty()) return;

    CreatureMesh mesh;
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = vertices.size() * sizeof(ProceduralMeshGenerator::Vertex);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.vertexBuffer));

    mesh.vbView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vbView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.vbView.StrideInBytes = sizeof(ProceduralMeshGenerator::Vertex);

    bufferDesc.Width = indices.size() * sizeof(uint16_t);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                     IID_PPV_ARGS(&mesh.indexBuffer));

    mesh.ibView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.ibView.SizeInBytes = static_cast<UINT>(bufferDesc.Width);
    mesh.ibView.Format = DXGI_FORMAT_R16_UINT;

    meshes_[static_cast<int>(type)] = std::move(mesh);
}

void SmallCreatureRenderer::createShaders(ID3D12Device* device) {
    // Root signature for instanced rendering
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;  // Would define constants/CBVs here
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // In a full implementation, would compile root signature and create PSOs
    // This is placeholder for the structure
}

void SmallCreatureRenderer::createInstanceBuffers(ID3D12Device* device) {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = MAX_INSTANCES * sizeof(SmallCreatureInstance);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                     IID_PPV_ARGS(&instanceBuffer_));

    // Point sprite buffer
    bufferDesc.Width = MAX_POINT_SPRITES * sizeof(PointSpriteInstance);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                     IID_PPV_ARGS(&pointSpriteBuffer_));

    // Particle buffer
    bufferDesc.Width = MAX_PARTICLES * sizeof(SwarmParticle);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                     IID_PPV_ARGS(&particleBuffer_));
}

void SmallCreatureRenderer::uploadInstanceData(ID3D12GraphicsCommandList* cmdList) {
    // Upload instance data
    if (!instances_.empty() && instanceBuffer_) {
        void* mappedData;
        instanceBuffer_->Map(0, nullptr, &mappedData);
        memcpy(mappedData, instances_.data(),
               std::min(instances_.size(), MAX_INSTANCES) * sizeof(SmallCreatureInstance));
        instanceBuffer_->Unmap(0, nullptr);
    }

    // Upload point sprites
    if (!pointSprites_.empty() && pointSpriteBuffer_) {
        void* mappedData;
        pointSpriteBuffer_->Map(0, nullptr, &mappedData);
        memcpy(mappedData, pointSprites_.data(),
               std::min(pointSprites_.size(), MAX_POINT_SPRITES) * sizeof(PointSpriteInstance));
        pointSpriteBuffer_->Unmap(0, nullptr);
    }
}

void SmallCreatureRenderer::sortAndBatch() {
    // Sort instances by type and LOD for batching
    std::sort(instances_.begin(), instances_.end(),
        [](const SmallCreatureInstance& a, const SmallCreatureInstance& b) {
            if (a.params.z != b.params.z) return a.params.z < b.params.z;  // Type
            return a.params.w < b.params.w;  // LOD
        });

    // Create batches
    batches_.clear();

    if (instances_.empty()) return;

    SmallCreatureType currentType = static_cast<SmallCreatureType>(static_cast<int>(instances_[0].params.z));
    LODLevel currentLOD = static_cast<LODLevel>(static_cast<int>(instances_[0].params.w));
    uint32_t batchStart = 0;

    for (size_t i = 1; i < instances_.size(); ++i) {
        SmallCreatureType type = static_cast<SmallCreatureType>(static_cast<int>(instances_[i].params.z));
        LODLevel lod = static_cast<LODLevel>(static_cast<int>(instances_[i].params.w));

        if (type != currentType || lod != currentLOD) {
            // End current batch
            RenderBatch batch;
            batch.type = currentType;
            batch.lod = currentLOD;
            batch.startIndex = batchStart;
            batch.instanceCount = static_cast<uint32_t>(i - batchStart);
            batches_.push_back(batch);

            // Start new batch
            currentType = type;
            currentLOD = lod;
            batchStart = static_cast<uint32_t>(i);
        }
    }

    // Final batch
    RenderBatch batch;
    batch.type = currentType;
    batch.lod = currentLOD;
    batch.startIndex = batchStart;
    batch.instanceCount = static_cast<uint32_t>(instances_.size() - batchStart);
    batches_.push_back(batch);
}

LODLevel SmallCreatureRenderer::calculateLOD(float distance) const {
    if (distance < detailedLODDist_) return LODLevel::DETAILED;
    if (distance < simplifiedLODDist_) return LODLevel::SIMPLIFIED;
    if (distance < pointLODDist_) return LODLevel::POINT;
    return LODLevel::PARTICLE;
}

float SmallCreatureRenderer::getMeshScaleFactor(SmallCreatureType type) const {
    // Procedural meshes are authored at approximately 1-unit scale
    // This factor converts them to proper world-scale meters
    // Different creature categories have different mesh complexity, so we adjust accordingly

    if (isInsect(type)) {
        // Insect meshes: body segments ~0.2-0.5 units, scale to actual mm-cm sizes
        return 1.0f;  // Direct 1:1 mapping (mesh units = world meters)
    }
    else if (isArachnid(type)) {
        // Spider/scorpion meshes: body ~0.3-0.8 units
        return 1.0f;
    }
    else if (isSmallMammal(type)) {
        // Mammal meshes: body ~0.4-0.6 units length
        return 1.0f;
    }
    else if (isReptile(type) || isAmphibian(type)) {
        // Reptile/amphibian meshes: body ~0.3-0.6 units
        return 1.0f;
    }

    return 1.0f;  // Default: assume mesh units = world meters
}

float SmallCreatureRenderer::getVisibilityBias(SizeCategory category) const {
    // Apply a visibility bias for point sprites to keep tiny creatures visible
    // at distance without making them appear unrealistically large
    // These multipliers are tuned for visual clarity, not physical accuracy
    //
    // The bias creates a "perceived size" that scales with distance:
    // - Very close: actual size dominates, bias has minimal effect
    // - Mid distance: bias helps tiny creatures remain visible as small dots
    // - Far distance: all creatures become similar-sized particles anyway
    //
    // Values calibrated for typical camera distances of 10-50m viewing
    switch (category) {
        case SizeCategory::MICROSCOPIC:
            // < 1mm creatures (mites, tiny spiders) - significant boost
            // Without boost: invisible at any useful distance
            // With boost: small but visible dots at 10-30m
            return 80.0f;  // A 0.5mm mite renders as ~4cm point sprite
        case SizeCategory::TINY:
            // 1mm - 1cm creatures (ants, flies, beetles) - moderate boost
            // Without boost: barely visible at 10m, invisible at 20m
            // With boost: clearly visible dots up to 30m
            return 15.0f;  // A 5mm ant renders as ~7.5cm point sprite
        case SizeCategory::SMALL:
            // 1cm - 10cm creatures (frogs, mice, grasshoppers) - small boost
            // Without boost: visible at 20m but fades quickly
            // With boost: visible up to 50m
            return 4.0f;   // A 5cm frog renders as ~20cm point sprite
        case SizeCategory::MEDIUM:
            // 10cm - 30cm creatures (squirrels, rabbits) - minimal boost
            // Already visible at reasonable distances
            return 2.0f;   // A 20cm squirrel renders as ~40cm point sprite
        default:
            return 1.0f;
    }
}

CreatureMesh* SmallCreatureRenderer::getMesh(SmallCreatureType type, LODLevel lod) {
    int key = static_cast<int>(type);

    if (lod == LODLevel::SIMPLIFIED) {
        auto it = simplifiedMeshes_.find(key);
        if (it != simplifiedMeshes_.end()) {
            return &it->second;
        }
    }

    auto it = meshes_.find(key);
    if (it != meshes_.end()) {
        return &it->second;
    }

    // Fall back to generic mesh for category
    if (isInsect(type)) {
        it = meshes_.find(static_cast<int>(SmallCreatureType::ANT_WORKER));
    } else if (isArachnid(type)) {
        it = meshes_.find(static_cast<int>(SmallCreatureType::SPIDER_ORB_WEAVER));
    } else if (isSmallMammal(type)) {
        it = meshes_.find(static_cast<int>(SmallCreatureType::MOUSE));
    } else if (isReptile(type)) {
        it = meshes_.find(static_cast<int>(SmallCreatureType::LIZARD_SMALL));
    } else if (isAmphibian(type)) {
        it = meshes_.find(static_cast<int>(SmallCreatureType::FROG));
    }

    if (it != meshes_.end()) {
        return &it->second;
    }

    return nullptr;
}

std::string SmallCreatureRenderer::getHabitatDebugString() const {
    std::ostringstream ss;
    ss << "=== Small Creature Distribution ===" << "\n";

    // Habitat breakdown
    ss << "HABITAT:" << "\n";
    ss << "  Ground:      " << std::setw(5) << habitatStats_.groundCount << "\n";
    ss << "  Grass:       " << std::setw(5) << habitatStats_.grassCount << "\n";
    ss << "  Bush:        " << std::setw(5) << habitatStats_.bushCount << "\n";
    ss << "  Canopy/Tree: " << std::setw(5) << habitatStats_.canopyCount << "\n";
    ss << "  Aerial:      " << std::setw(5) << habitatStats_.aerialCount << "\n";
    ss << "  Underground: " << std::setw(5) << habitatStats_.undergroundCount << "\n";
    ss << "  Aquatic:     " << std::setw(5) << habitatStats_.aquaticCount << "\n";

    // Size breakdown
    ss << "SIZE CATEGORY:" << "\n";
    ss << "  Microscopic (<1mm): " << std::setw(5) << habitatStats_.microscopicCount << "\n";
    ss << "  Tiny (1-10mm):      " << std::setw(5) << habitatStats_.tinyCount << "\n";
    ss << "  Small (1-10cm):     " << std::setw(5) << habitatStats_.smallCount << "\n";
    ss << "  Medium (10-30cm):   " << std::setw(5) << habitatStats_.mediumCount << "\n";

    // LOD breakdown
    ss << "RENDERING:" << "\n";
    ss << "  Detailed:    " << std::setw(5) << detailedCount_ << "\n";
    ss << "  Simplified:  " << std::setw(5) << simplifiedCount_ << "\n";
    ss << "  Point:       " << std::setw(5) << pointCount_ << "\n";
    ss << "  Particle:    " << std::setw(5) << particleCount_ << "\n";
    ss << "  Total:       " << std::setw(5) << totalInstances_ << "\n";

    return ss.str();
}

// =============================================================================
// SwarmVisualizer Implementation
// =============================================================================

SwarmVisualizer::SwarmVisualizer()
    : rng_(std::random_device{}()) {
}

SwarmVisualizer::~SwarmVisualizer() = default;

bool SwarmVisualizer::initialize(ID3D12Device* device) {
    return true;
}

void SwarmVisualizer::update(float deltaTime, const std::vector<SmallCreature>& creatures) {
    // Update existing particles
    for (auto& p : particles_) {
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.position.z += p.velocity.z * deltaTime;
        p.life -= deltaTime;
    }

    // Remove dead particles
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [](const SwarmParticle& p) { return p.life <= 0.0f; }),
        particles_.end()
    );
}

void SwarmVisualizer::generateSwarmParticles(const XMFLOAT3& center, int count,
                                              SmallCreatureType type, const XMFLOAT4& color) {
    std::uniform_real_distribution<float> posDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> lifeDist(1.0f, 3.0f);

    for (int i = 0; i < count; ++i) {
        SwarmParticle p;
        p.position = {
            center.x + posDist(rng_),
            center.y + posDist(rng_),
            center.z + posDist(rng_)
        };
        p.velocity = {
            velDist(rng_),
            velDist(rng_) * 0.5f,
            velDist(rng_)
        };
        p.size = 0.05f;
        p.life = lifeDist(rng_);
        p.color = color;

        particles_.push_back(p);
    }
}

// =============================================================================
// ProceduralMeshGenerator Implementation
// =============================================================================

void ProceduralMeshGenerator::generateInsectBody(std::vector<Vertex>& vertices,
                                                  std::vector<uint16_t>& indices,
                                                  float headSize, float thoraxSize,
                                                  float abdomenSize) {
    vertices.clear();
    indices.clear();

    // Head
    addEllipsoid(vertices, indices, { 0.5f, 0.0f, 0.0f },
                 { headSize, headSize * 0.8f, headSize * 0.8f }, 8, 6);

    // Thorax (middle segment)
    addEllipsoid(vertices, indices, { 0.0f, 0.0f, 0.0f },
                 { thoraxSize, thoraxSize * 0.7f, thoraxSize * 0.8f }, 8, 6);

    // Abdomen (rear segment)
    addEllipsoid(vertices, indices, { -0.5f, 0.0f, 0.0f },
                 { abdomenSize, abdomenSize * 0.6f, abdomenSize * 0.7f }, 8, 6);

    // Legs (6 legs from thorax)
    float legRadius = 0.02f;
    for (int side = -1; side <= 1; side += 2) {
        for (int leg = 0; leg < 3; ++leg) {
            float xOff = 0.1f - leg * 0.15f;
            XMFLOAT3 legStart = { xOff, -thoraxSize * 0.3f, side * thoraxSize * 0.4f };
            XMFLOAT3 legEnd = { xOff - 0.1f, -thoraxSize * 0.8f, side * thoraxSize * 0.8f };
            addCylinder(vertices, indices, legStart, legEnd, legRadius, 4);
        }
    }

    // Antennae
    XMFLOAT3 antStart1 = { 0.5f + headSize * 0.5f, headSize * 0.3f, headSize * 0.3f };
    XMFLOAT3 antEnd1 = { 0.7f, 0.3f, 0.4f };
    addCylinder(vertices, indices, antStart1, antEnd1, 0.01f, 3);

    XMFLOAT3 antStart2 = { 0.5f + headSize * 0.5f, headSize * 0.3f, -headSize * 0.3f };
    XMFLOAT3 antEnd2 = { 0.7f, 0.3f, -0.4f };
    addCylinder(vertices, indices, antStart2, antEnd2, 0.01f, 3);
}

void ProceduralMeshGenerator::generateSpiderBody(std::vector<Vertex>& vertices,
                                                  std::vector<uint16_t>& indices,
                                                  float bodySize) {
    vertices.clear();
    indices.clear();

    // Cephalothorax (front)
    addEllipsoid(vertices, indices, { 0.2f, 0.0f, 0.0f },
                 { bodySize * 0.6f, bodySize * 0.4f, bodySize * 0.5f }, 8, 6);

    // Abdomen (rear, larger)
    addEllipsoid(vertices, indices, { -0.3f, 0.0f, 0.0f },
                 { bodySize * 0.8f, bodySize * 0.6f, bodySize * 0.7f }, 8, 6);

    // 8 legs
    float legRadius = bodySize * 0.05f;
    for (int side = -1; side <= 1; side += 2) {
        for (int leg = 0; leg < 4; ++leg) {
            float xOff = 0.3f - leg * 0.15f;
            float angle = leg * 0.3f + 0.2f;

            XMFLOAT3 legStart = { xOff, 0.0f, side * bodySize * 0.3f };
            XMFLOAT3 legEnd = {
                xOff - cosf(angle) * bodySize * 0.8f,
                -sinf(angle) * bodySize * 0.6f,
                side * bodySize * 1.0f
            };
            addCylinder(vertices, indices, legStart, legEnd, legRadius, 4);
        }
    }
}

void ProceduralMeshGenerator::generateMammalBody(std::vector<Vertex>& vertices,
                                                  std::vector<uint16_t>& indices,
                                                  float bodyLength, float bodyWidth) {
    vertices.clear();
    indices.clear();

    // Body
    addEllipsoid(vertices, indices, { 0.0f, 0.0f, 0.0f },
                 { bodyLength * 0.5f, bodyWidth * 0.4f, bodyWidth * 0.5f }, 10, 8);

    // Head
    float headSize = bodyWidth * 0.4f;
    addEllipsoid(vertices, indices, { bodyLength * 0.5f, 0.0f, 0.0f },
                 { headSize, headSize * 0.7f, headSize * 0.8f }, 8, 6);

    // Legs (4)
    float legRadius = bodyWidth * 0.08f;
    float legLength = bodyWidth * 0.6f;
    for (int side = -1; side <= 1; side += 2) {
        // Front legs
        XMFLOAT3 flStart = { bodyLength * 0.3f, -bodyWidth * 0.2f, side * bodyWidth * 0.3f };
        XMFLOAT3 flEnd = { bodyLength * 0.3f, -legLength, side * bodyWidth * 0.3f };
        addCylinder(vertices, indices, flStart, flEnd, legRadius, 4);

        // Back legs
        XMFLOAT3 blStart = { -bodyLength * 0.3f, -bodyWidth * 0.2f, side * bodyWidth * 0.3f };
        XMFLOAT3 blEnd = { -bodyLength * 0.3f, -legLength, side * bodyWidth * 0.3f };
        addCylinder(vertices, indices, blStart, blEnd, legRadius, 4);
    }

    // Tail
    XMFLOAT3 tailStart = { -bodyLength * 0.5f, 0.0f, 0.0f };
    XMFLOAT3 tailEnd = { -bodyLength * 0.9f, bodyWidth * 0.2f, 0.0f };
    addCone(vertices, indices, tailStart, tailEnd, bodyWidth * 0.1f, 4);
}

void ProceduralMeshGenerator::generateFrogBody(std::vector<Vertex>& vertices,
                                                std::vector<uint16_t>& indices,
                                                float bodySize) {
    vertices.clear();
    indices.clear();

    // Body (squat)
    addEllipsoid(vertices, indices, { 0.0f, 0.0f, 0.0f },
                 { bodySize * 0.6f, bodySize * 0.4f, bodySize * 0.8f }, 10, 8);

    // Head (wide)
    addEllipsoid(vertices, indices, { bodySize * 0.5f, bodySize * 0.1f, 0.0f },
                 { bodySize * 0.4f, bodySize * 0.3f, bodySize * 0.5f }, 8, 6);

    // Eyes (bulging)
    addEllipsoid(vertices, indices, { bodySize * 0.6f, bodySize * 0.3f, bodySize * 0.3f },
                 { bodySize * 0.1f, bodySize * 0.12f, bodySize * 0.1f }, 6, 4);
    addEllipsoid(vertices, indices, { bodySize * 0.6f, bodySize * 0.3f, -bodySize * 0.3f },
                 { bodySize * 0.1f, bodySize * 0.12f, bodySize * 0.1f }, 6, 4);

    // Back legs (powerful)
    float legRadius = bodySize * 0.08f;
    for (int side = -1; side <= 1; side += 2) {
        // Thigh
        XMFLOAT3 thighStart = { -bodySize * 0.2f, -bodySize * 0.2f, side * bodySize * 0.4f };
        XMFLOAT3 thighEnd = { -bodySize * 0.5f, -bodySize * 0.1f, side * bodySize * 0.8f };
        addCylinder(vertices, indices, thighStart, thighEnd, legRadius * 1.2f, 4);

        // Lower leg
        XMFLOAT3 lowerStart = thighEnd;
        XMFLOAT3 lowerEnd = { -bodySize * 0.3f, -bodySize * 0.4f, side * bodySize * 0.6f };
        addCylinder(vertices, indices, lowerStart, lowerEnd, legRadius, 4);
    }

    // Front legs (smaller)
    for (int side = -1; side <= 1; side += 2) {
        XMFLOAT3 legStart = { bodySize * 0.2f, -bodySize * 0.2f, side * bodySize * 0.3f };
        XMFLOAT3 legEnd = { bodySize * 0.3f, -bodySize * 0.4f, side * bodySize * 0.4f };
        addCylinder(vertices, indices, legStart, legEnd, legRadius * 0.8f, 4);
    }
}

void ProceduralMeshGenerator::generateSimplifiedInsect(std::vector<Vertex>& vertices,
                                                        std::vector<uint16_t>& indices) {
    vertices.clear();
    indices.clear();

    // Simple 3-segment body
    addEllipsoid(vertices, indices, { 0.3f, 0.0f, 0.0f }, { 0.15f, 0.1f, 0.1f }, 4, 3);
    addEllipsoid(vertices, indices, { 0.0f, 0.0f, 0.0f }, { 0.2f, 0.12f, 0.12f }, 4, 3);
    addEllipsoid(vertices, indices, { -0.3f, 0.0f, 0.0f }, { 0.25f, 0.15f, 0.15f }, 4, 3);
}

void ProceduralMeshGenerator::generateSimplifiedQuadruped(std::vector<Vertex>& vertices,
                                                           std::vector<uint16_t>& indices) {
    vertices.clear();
    indices.clear();

    // Simple body + head
    addEllipsoid(vertices, indices, { 0.0f, 0.0f, 0.0f }, { 0.4f, 0.2f, 0.25f }, 6, 4);
    addEllipsoid(vertices, indices, { 0.35f, 0.05f, 0.0f }, { 0.15f, 0.12f, 0.12f }, 4, 3);
}

void ProceduralMeshGenerator::addEllipsoid(std::vector<Vertex>& vertices,
                                            std::vector<uint16_t>& indices,
                                            const XMFLOAT3& center, const XMFLOAT3& radii,
                                            int segments, int rings) {
    uint16_t baseIndex = static_cast<uint16_t>(vertices.size());

    // Generate vertices
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = XM_PI * ring / rings;
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int seg = 0; seg <= segments; ++seg) {
            float theta = 2.0f * XM_PI * seg / segments;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            Vertex v;
            v.position = {
                center.x + radii.x * sinPhi * cosTheta,
                center.y + radii.y * cosPhi,
                center.z + radii.z * sinPhi * sinTheta
            };

            // Normal (not exactly correct for ellipsoid, but close enough)
            v.normal = {
                sinPhi * cosTheta / radii.x,
                cosPhi / radii.y,
                sinPhi * sinTheta / radii.z
            };
            float len = sqrtf(v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z);
            v.normal.x /= len;
            v.normal.y /= len;
            v.normal.z /= len;

            v.texCoord = { static_cast<float>(seg) / segments, static_cast<float>(ring) / rings };

            vertices.push_back(v);
        }
    }

    // Generate indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            uint16_t current = baseIndex + ring * (segments + 1) + seg;
            uint16_t next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

void ProceduralMeshGenerator::addCylinder(std::vector<Vertex>& vertices,
                                           std::vector<uint16_t>& indices,
                                           const XMFLOAT3& start, const XMFLOAT3& end,
                                           float radius, int segments) {
    uint16_t baseIndex = static_cast<uint16_t>(vertices.size());

    // Direction vector
    XMFLOAT3 dir = { end.x - start.x, end.y - start.y, end.z - start.z };
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    dir.x /= length;
    dir.y /= length;
    dir.z /= length;

    // Perpendicular vectors
    XMFLOAT3 perp1, perp2;
    if (fabsf(dir.y) < 0.9f) {
        perp1 = { -dir.z, 0, dir.x };
    } else {
        perp1 = { 1, 0, 0 };
    }
    float perpLen = sqrtf(perp1.x * perp1.x + perp1.y * perp1.y + perp1.z * perp1.z);
    perp1.x /= perpLen;
    perp1.y /= perpLen;
    perp1.z /= perpLen;

    perp2.x = dir.y * perp1.z - dir.z * perp1.y;
    perp2.y = dir.z * perp1.x - dir.x * perp1.z;
    perp2.z = dir.x * perp1.y - dir.y * perp1.x;

    // Generate vertices for both ends
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * XM_PI * i / segments;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        XMFLOAT3 offset = {
            (perp1.x * cosA + perp2.x * sinA) * radius,
            (perp1.y * cosA + perp2.y * sinA) * radius,
            (perp1.z * cosA + perp2.z * sinA) * radius
        };

        // Start cap vertex
        Vertex v1;
        v1.position = { start.x + offset.x, start.y + offset.y, start.z + offset.z };
        v1.normal = { offset.x / radius, offset.y / radius, offset.z / radius };
        v1.texCoord = { static_cast<float>(i) / segments, 0.0f };
        vertices.push_back(v1);

        // End cap vertex
        Vertex v2;
        v2.position = { end.x + offset.x, end.y + offset.y, end.z + offset.z };
        v2.normal = v1.normal;
        v2.texCoord = { static_cast<float>(i) / segments, 1.0f };
        vertices.push_back(v2);
    }

    // Generate indices
    for (int i = 0; i < segments; ++i) {
        uint16_t i1 = baseIndex + i * 2;
        uint16_t i2 = baseIndex + i * 2 + 1;
        uint16_t i3 = baseIndex + (i + 1) * 2;
        uint16_t i4 = baseIndex + (i + 1) * 2 + 1;

        indices.push_back(i1);
        indices.push_back(i3);
        indices.push_back(i2);

        indices.push_back(i2);
        indices.push_back(i3);
        indices.push_back(i4);
    }
}

void ProceduralMeshGenerator::addCone(std::vector<Vertex>& vertices,
                                       std::vector<uint16_t>& indices,
                                       const XMFLOAT3& base, const XMFLOAT3& tip,
                                       float radius, int segments) {
    uint16_t baseIndex = static_cast<uint16_t>(vertices.size());

    // Direction vector
    XMFLOAT3 dir = { tip.x - base.x, tip.y - base.y, tip.z - base.z };
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    dir.x /= length;
    dir.y /= length;
    dir.z /= length;

    // Perpendicular vectors
    XMFLOAT3 perp1, perp2;
    if (fabsf(dir.y) < 0.9f) {
        perp1 = { -dir.z, 0, dir.x };
    } else {
        perp1 = { 1, 0, 0 };
    }
    float perpLen = sqrtf(perp1.x * perp1.x + perp1.y * perp1.y + perp1.z * perp1.z);
    perp1.x /= perpLen;
    perp1.y /= perpLen;
    perp1.z /= perpLen;

    perp2.x = dir.y * perp1.z - dir.z * perp1.y;
    perp2.y = dir.z * perp1.x - dir.x * perp1.z;
    perp2.z = dir.x * perp1.y - dir.y * perp1.x;

    // Tip vertex
    Vertex tipV;
    tipV.position = tip;
    tipV.normal = dir;
    tipV.texCoord = { 0.5f, 0.0f };
    vertices.push_back(tipV);

    // Base vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * XM_PI * i / segments;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        XMFLOAT3 offset = {
            (perp1.x * cosA + perp2.x * sinA) * radius,
            (perp1.y * cosA + perp2.y * sinA) * radius,
            (perp1.z * cosA + perp2.z * sinA) * radius
        };

        Vertex v;
        v.position = { base.x + offset.x, base.y + offset.y, base.z + offset.z };
        v.normal = { offset.x / radius, offset.y / radius, offset.z / radius };
        v.texCoord = { static_cast<float>(i) / segments, 1.0f };
        vertices.push_back(v);
    }

    // Generate indices (fan from tip)
    for (int i = 0; i < segments; ++i) {
        indices.push_back(baseIndex);  // Tip
        indices.push_back(baseIndex + 1 + i);
        indices.push_back(baseIndex + 2 + i);
    }
}

} // namespace small
