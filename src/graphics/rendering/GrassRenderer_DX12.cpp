#include "GrassRenderer_DX12.h"
#include <iostream>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Grass blade vertex structure (billboard quad)
struct GrassBladeVertex {
    float position[3];  // Local position (-0.5 to 0.5)
    float texCoord[2];  // UV coordinates
};

static_assert(sizeof(GrassBladeVertex) == 20, "GrassBladeVertex must be 20 bytes");

// Maximum grass blades per frame
static constexpr uint32_t MAX_GRASS_INSTANCES = 300000;

GrassRendererDX12::GrassRendererDX12()
    : m_device(nullptr)
    , m_grassSystem(nullptr)
    , m_visibleCount(0)
{
}

GrassRendererDX12::~GrassRendererDX12() {
    // UniquePtr handles cleanup
}

bool GrassRendererDX12::init(IDevice* device, GrassSystem* grassSystem) {
    m_device = device;
    m_grassSystem = grassSystem;

    if (!m_device) {
        std::cerr << "[GrassRendererDX12] ERROR: Null device!" << std::endl;
        return false;
    }

    if (!m_grassSystem) {
        std::cerr << "[GrassRendererDX12] WARNING: No grass system provided" << std::endl;
    }

    // Create blade geometry (billboard quad)
    if (!createBladeGeometry()) {
        std::cerr << "[GrassRendererDX12] ERROR: Failed to create blade geometry!" << std::endl;
        return false;
    }

    // Create instance buffer
    BufferDesc instBufDesc;
    instBufDesc.size = MAX_GRASS_INSTANCES * sizeof(GrassInstanceGPU);
    instBufDesc.usage = BufferUsage::Vertex;
    instBufDesc.cpuAccess = true;  // Updated every frame
    instBufDesc.debugName = "GrassInstanceBuffer";

    m_instanceBuffer = m_device->CreateBuffer(instBufDesc);
    if (!m_instanceBuffer) {
        std::cerr << "[GrassRendererDX12] ERROR: Failed to create instance buffer!" << std::endl;
        return false;
    }

    // Create constant buffer
    BufferDesc cbDesc;
    cbDesc.size = sizeof(GrassConstants);
    cbDesc.usage = BufferUsage::Uniform;
    cbDesc.cpuAccess = true;
    cbDesc.debugName = "GrassConstantBuffer";

    m_constantBuffer = m_device->CreateBuffer(cbDesc);
    if (!m_constantBuffer) {
        std::cerr << "[GrassRendererDX12] ERROR: Failed to create constant buffer!" << std::endl;
        return false;
    }

    // Reserve space for visible instances
    m_visibleInstances.reserve(MAX_GRASS_INSTANCES);

    std::cout << "[GrassRendererDX12] Initialized successfully" << std::endl;
    std::cout << "  Max instances: " << MAX_GRASS_INSTANCES << std::endl;

    return true;
}

bool GrassRendererDX12::createBladeGeometry() {
    // Create a simple quad for grass blade billboard
    // Vertices are in local space, transformed in shader
    GrassBladeVertex vertices[] = {
        // Position               // TexCoord
        {{-0.5f, 0.0f, 0.0f},    {0.0f, 0.0f}},  // Bottom-left
        {{ 0.5f, 0.0f, 0.0f},    {1.0f, 0.0f}},  // Bottom-right
        {{ 0.5f, 1.0f, 0.0f},    {1.0f, 1.0f}},  // Top-right
        {{-0.5f, 1.0f, 0.0f},    {0.0f, 1.0f}},  // Top-left
    };

    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // Create vertex buffer
    BufferDesc vbDesc;
    vbDesc.size = sizeof(vertices);
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuAccess = true;
    vbDesc.debugName = "GrassBladeVertexBuffer";

    m_bladeVertexBuffer = m_device->CreateBuffer(vbDesc);
    if (!m_bladeVertexBuffer) {
        return false;
    }

    // Upload vertex data
    void* vbData = m_bladeVertexBuffer->Map();
    if (!vbData) {
        return false;
    }
    memcpy(vbData, vertices, sizeof(vertices));
    m_bladeVertexBuffer->Unmap();

    // Create index buffer
    BufferDesc ibDesc;
    ibDesc.size = sizeof(indices);
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuAccess = true;
    ibDesc.debugName = "GrassBladeIndexBuffer";

    m_bladeIndexBuffer = m_device->CreateBuffer(ibDesc);
    if (!m_bladeIndexBuffer) {
        return false;
    }

    // Upload index data
    void* ibData = m_bladeIndexBuffer->Map();
    if (!ibData) {
        return false;
    }
    memcpy(ibData, indices, sizeof(indices));
    m_bladeIndexBuffer->Unmap();

    return true;
}

void GrassRendererDX12::setWindParams(const glm::vec2& direction, float strength) {
    m_windDirection = glm::normalize(direction);
    m_windStrength = strength;
}

void GrassRendererDX12::updateInstances(const glm::vec3& cameraPos) {
    m_visibleInstances.clear();
    m_visibleClusters.clear();
    m_individualCount = 0;
    m_clusteredCount = 0;
    m_texturedCount = 0;

    if (!m_grassSystem) {
        m_visibleCount = 0;
        m_clusterCount = 0;
        return;
    }

    // Get all grass instances from the system
    const std::vector<GrassBladeInstance>& allInstances = m_grassSystem->getInstances();

    // Filter by distance and build GPU data with LOD levels
    for (const auto& blade : allInstances) {
        float distance = glm::distance(cameraPos, blade.position);

        // Determine LOD level
        LOD::GrassLOD lod = LOD::calculateGrassLOD(distance, m_lodConfig);

        // Skip if culled
        if (lod == LOD::GrassLOD::CULLED) {
            continue;
        }

        // Calculate fade factor for smooth transitions
        float fadeFactor = LOD::calculateGrassFade(distance, lod, m_lodConfig);

        // Handle different LOD levels
        if (lod == LOD::GrassLOD::INDIVIDUAL) {
            // Full individual blade rendering (close range)
            GrassInstanceGPU gpuInstance;
            gpuInstance.position[0] = blade.position.x;
            gpuInstance.position[1] = blade.position.y;
            gpuInstance.position[2] = blade.position.z;
            gpuInstance.rotation = blade.rotation;
            gpuInstance.height = blade.height;
            gpuInstance.width = blade.width;
            gpuInstance.bendFactor = blade.bendFactor;
            gpuInstance.colorVariation = blade.colorVariation;
            gpuInstance.distance = distance;
            gpuInstance.fadeFactor = fadeFactor;
            gpuInstance.lodLevel = static_cast<uint32_t>(lod);
            gpuInstance.padding = 0.0f;

            m_visibleInstances.push_back(gpuInstance);
            m_individualCount++;
        }
        else if (lod == LOD::GrassLOD::CLUSTERED) {
            // Medium LOD: reduce density but still render individual blades
            // Use spatial hash to consistently skip some blades
            uint32_t hash = static_cast<uint32_t>(blade.position.x * 1000.0f) ^
                           static_cast<uint32_t>(blade.position.z * 1000.0f);
            float random = (hash % 100) / 100.0f;

            // Render 50% of blades at clustered LOD
            if (random < 0.5f) {
                GrassInstanceGPU gpuInstance;
                gpuInstance.position[0] = blade.position.x;
                gpuInstance.position[1] = blade.position.y;
                gpuInstance.position[2] = blade.position.z;
                gpuInstance.rotation = blade.rotation;
                gpuInstance.height = blade.height * 1.2f;  // Slightly taller to compensate
                gpuInstance.width = blade.width * 1.5f;    // Wider to fill gaps
                gpuInstance.bendFactor = blade.bendFactor;
                gpuInstance.colorVariation = blade.colorVariation;
                gpuInstance.distance = distance;
                gpuInstance.fadeFactor = fadeFactor;
                gpuInstance.lodLevel = static_cast<uint32_t>(lod);
                gpuInstance.padding = 0.0f;

                m_visibleInstances.push_back(gpuInstance);
                m_clusteredCount++;
            }
        }
        else if (lod == LOD::GrassLOD::TEXTURED) {
            // Far LOD: very sparse, larger blades
            uint32_t hash = static_cast<uint32_t>(blade.position.x * 100.0f) ^
                           static_cast<uint32_t>(blade.position.z * 100.0f);
            float random = (hash % 100) / 100.0f;

            // Render 10% of blades at textured LOD, but make them much larger
            if (random < 0.1f) {
                GrassInstanceGPU gpuInstance;
                gpuInstance.position[0] = blade.position.x;
                gpuInstance.position[1] = blade.position.y;
                gpuInstance.position[2] = blade.position.z;
                gpuInstance.rotation = blade.rotation;
                gpuInstance.height = blade.height * 2.0f;
                gpuInstance.width = blade.width * 3.0f;
                gpuInstance.bendFactor = blade.bendFactor * 0.5f;  // Less bend at distance
                gpuInstance.colorVariation = blade.colorVariation;
                gpuInstance.distance = distance;
                gpuInstance.fadeFactor = fadeFactor;
                gpuInstance.lodLevel = static_cast<uint32_t>(lod);
                gpuInstance.padding = 0.0f;

                m_visibleInstances.push_back(gpuInstance);
                m_texturedCount++;
            }
        }

        // Cap at maximum
        if (m_visibleInstances.size() >= MAX_GRASS_INSTANCES) {
            break;
        }
    }

    m_visibleCount = static_cast<int>(m_visibleInstances.size());

    // Build clusters for medium-distance rendering optimization
    buildGrassClusters(cameraPos);
}

void GrassRendererDX12::buildGrassClusters(const glm::vec3& cameraPos) {
    // This would build cluster data for even more optimized rendering
    // For now, we use the density-reduced individual blades approach
    m_clusterCount = static_cast<int>(m_visibleClusters.size());
}

void GrassRendererDX12::updateInstanceBuffer() {
    if (m_visibleInstances.empty() || !m_instanceBuffer) {
        return;
    }

    void* data = m_instanceBuffer->Map();
    if (data) {
        memcpy(data, m_visibleInstances.data(),
               m_visibleInstances.size() * sizeof(GrassInstanceGPU));
        m_instanceBuffer->Unmap();
    }
}

void GrassRendererDX12::render(
    ICommandList* cmdList,
    IPipeline* pipeline,
    const glm::mat4& viewProj,
    const glm::vec3& cameraPos,
    const glm::vec3& lightDir,
    const glm::vec3& lightColor,
    float time
) {
    if (m_visibleCount == 0) {
        return;
    }

    // Update instance buffer with visible grass
    updateInstanceBuffer();

    // Get grass colors from the system
    GrassConfig config;
    if (m_grassSystem) {
        config = GrassSystem::getConfigForBiome(0);  // Default biome
    }

    // Update constant buffer
    GrassConstants constants;
    memset(&constants, 0, sizeof(constants));

    // View-projection matrix supplied by caller (keeps camera consistent across passes)
    memcpy(constants.viewProj, glm::value_ptr(viewProj), sizeof(float) * 16);

    // Camera position - use public member
    constants.cameraPos[0] = cameraPos.x;
    constants.cameraPos[1] = cameraPos.y;
    constants.cameraPos[2] = cameraPos.z;
    constants.cameraPos[3] = 0.0f;

    // Wind direction and time
    constants.windDir[0] = m_windDirection.x;
    constants.windDir[1] = m_windDirection.y;
    constants.windDir[2] = m_windStrength;
    constants.windDir[3] = time;

    // Grass colors (base color + tip color)
    constants.grassColors[0] = config.baseColor.r;
    constants.grassColors[1] = config.baseColor.g;
    constants.grassColors[2] = config.baseColor.b;
    constants.grassColors[3] = 1.0f;  // Padding
    constants.grassColors[4] = config.tipColor.r;
    constants.grassColors[5] = config.tipColor.g;
    constants.grassColors[6] = config.tipColor.b;
    constants.grassColors[7] = 1.0f;  // Padding

    // Light direction
    constants.lightDir[0] = lightDir.x;
    constants.lightDir[1] = lightDir.y;
    constants.lightDir[2] = lightDir.z;
    constants.lightDir[3] = 0.0f;

    // Light color
    constants.lightColor[0] = lightColor.x;
    constants.lightColor[1] = lightColor.y;
    constants.lightColor[2] = lightColor.z;
    constants.lightColor[3] = 1.0f;

    // LOD parameters from config
    constants.lodParams[0] = m_lodConfig.grassIndividual;
    constants.lodParams[1] = m_lodConfig.grassClustered;
    constants.lodParams[2] = m_lodConfig.grassMaxDistance;
    constants.lodParams[3] = m_lodConfig.grassFadeRange;

    // Fog parameters for smooth LOD transitions
    constants.fogParams[0] = m_lodConfig.fogStart;
    constants.fogParams[1] = m_lodConfig.fogEnd;
    constants.fogParams[2] = m_lodConfig.fogDensity;
    constants.fogParams[3] = 0.0f;

    // Fog color
    constants.fogColor[0] = m_lodConfig.fogColor.x;
    constants.fogColor[1] = m_lodConfig.fogColor.y;
    constants.fogColor[2] = m_lodConfig.fogColor.z;
    constants.fogColor[3] = 0.0f;

    // Upload constant buffer
    void* cbData = m_constantBuffer->Map();
    if (cbData) {
        memcpy(cbData, &constants, sizeof(GrassConstants));
        m_constantBuffer->Unmap();
    }

    // Set pipeline
    cmdList->SetPipeline(pipeline);

    // Bind constant buffer
    cmdList->BindConstantBuffer(0, m_constantBuffer.get());

    // Bind vertex buffers:
    // Slot 0: Per-vertex blade geometry
    // Slot 1: Per-instance grass data
    cmdList->BindVertexBuffer(0, m_bladeVertexBuffer.get(), sizeof(GrassBladeVertex), 0);
    cmdList->BindVertexBuffer(1, m_instanceBuffer.get(), sizeof(GrassInstanceGPU), 0);

    // Bind index buffer
    cmdList->BindIndexBuffer(m_bladeIndexBuffer.get(), IndexFormat::UInt32, 0);

    // Draw all visible grass instances
    cmdList->DrawIndexedInstanced(6, static_cast<uint32_t>(m_visibleCount), 0, 0, 0);
}
