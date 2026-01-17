// TerrainRenderer_DX12.cpp
// Renders heightmap-based terrain using procedural noise generation.
// This implementation is self-contained and does not depend on external height providers.

#include "TerrainRenderer_DX12.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

// Perlin-like noise for terrain generation
namespace TerrainNoise {
    inline float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    inline float grad(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    // Simple permutation table
    static const int perm[512] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    inline float perlin2D(float x, float y) {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        float u = fade(x);
        float v = fade(y);

        int A = perm[X] + Y;
        int B = perm[X + 1] + Y;

        return lerp(
            lerp(grad(perm[A], x, y), grad(perm[B], x - 1, y), u),
            lerp(grad(perm[A + 1], x, y - 1), grad(perm[B + 1], x - 1, y - 1), u),
            v
        );
    }

    inline float octaveNoise(float x, float y, int octaves, float persistence) {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            total += perlin2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return (total / maxValue + 1.0f) * 0.5f; // Normalize to [0, 1]
    }

    inline float smoothstep(float edge0, float edge1, float x) {
        float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    }
}

// Biome color constants (normalized height thresholds)
namespace BiomeConfig {
    constexpr float WATER_LEVEL = 0.35f;
    constexpr float BEACH_LEVEL = 0.42f;
    constexpr float GRASS_LEVEL = 0.65f;
    constexpr float FOREST_LEVEL = 0.80f;
    constexpr float SNOW_LEVEL = 0.92f;

    // Biome colors
    const glm::vec3 WATER_COLOR = glm::vec3(0.2f, 0.4f, 0.8f);
    const glm::vec3 BEACH_COLOR = glm::vec3(0.9f, 0.85f, 0.6f);
    const glm::vec3 GRASS_COLOR = glm::vec3(0.3f, 0.7f, 0.3f);
    const glm::vec3 FOREST_COLOR = glm::vec3(0.2f, 0.5f, 0.2f);
    const glm::vec3 MOUNTAIN_COLOR = glm::vec3(0.6f, 0.6f, 0.6f);
    const glm::vec3 SNOW_COLOR = glm::vec3(0.95f, 0.95f, 1.0f);
}

TerrainRendererDX12::TerrainRendererDX12() = default;

TerrainRendererDX12::~TerrainRendererDX12() {
    shutdown();
}

bool TerrainRendererDX12::init(IDevice* device) {
    if (!device) {
        std::cerr << "TerrainRendererDX12: Invalid device" << std::endl;
        return false;
    }

    m_device = device;

    // Create upload command list and fence for static buffer uploads
    m_uploadCommandList = m_device->CreateCommandList(CommandListType::Graphics);
    if (!m_uploadCommandList) {
        std::cerr << "TerrainRendererDX12: Failed to create upload command list" << std::endl;
        return false;
    }

    m_uploadFence = m_device->CreateFence(0);
    if (!m_uploadFence) {
        std::cerr << "TerrainRendererDX12: Failed to create upload fence" << std::endl;
        return false;
    }

    // Create constant buffer
    if (!createConstantBuffer()) {
        std::cerr << "TerrainRendererDX12: Failed to create constant buffer" << std::endl;
        return false;
    }

    // Create chunk meshes
    if (!createChunkMeshes()) {
        std::cerr << "TerrainRendererDX12: Failed to create chunk meshes" << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "TerrainRendererDX12: Initialized with " << m_chunkMeshes.size()
              << " chunks, " << m_totalVertices << " vertices, "
              << m_totalIndices << " indices" << std::endl;

    return true;
}

void TerrainRendererDX12::shutdown() {
    if (m_device && m_uploadFence) {
        // Wait for any pending uploads
        m_device->WaitIdle();
    }

    m_chunkMeshes.clear();
    m_constantBuffer.reset();
    m_uploadCommandList.reset();
    m_uploadFence.reset();

    m_device = nullptr;
    m_initialized = false;
}

bool TerrainRendererDX12::createConstantBuffer() {
    BufferDesc cbDesc;
    cbDesc.size = sizeof(TerrainConstants);
    cbDesc.usage = BufferUsage::Uniform;
    cbDesc.cpuAccess = true;
    cbDesc.debugName = "TerrainConstantBuffer";

    m_constantBuffer = m_device->CreateBuffer(cbDesc);
    return m_constantBuffer != nullptr;
}

bool TerrainRendererDX12::createChunkMeshes() {
    using namespace TerrainRendererConfig;
    const int chunksPerAxis = CHUNKS_PER_AXIS;
    const int totalChunks = chunksPerAxis * chunksPerAxis;

    m_chunkMeshes.resize(totalChunks);
    m_totalVertices = 0;
    m_totalIndices = 0;

    std::vector<TerrainVertexDX12> vertices;
    std::vector<uint32_t> indices;

    for (int z = 0; z < chunksPerAxis; ++z) {
        for (int x = 0; x < chunksPerAxis; ++x) {
            int chunkIndex = z * chunksPerAxis + x;

            vertices.clear();
            indices.clear();

            if (!generateChunkMesh(x, z, vertices, indices)) {
                std::cerr << "TerrainRendererDX12: Failed to generate mesh for chunk ("
                          << x << ", " << z << ")" << std::endl;
                continue;
            }

            if (vertices.empty() || indices.empty()) {
                continue;
            }

            // Create vertex buffer
            BufferDesc vbDesc;
            vbDesc.size = vertices.size() * sizeof(TerrainVertexDX12);
            vbDesc.usage = BufferUsage::Vertex;
            vbDesc.cpuAccess = true;
            vbDesc.debugName = "TerrainChunkVB";

            m_chunkMeshes[chunkIndex].vertexBuffer = m_device->CreateBuffer(vbDesc);
            if (!m_chunkMeshes[chunkIndex].vertexBuffer) {
                std::cerr << "TerrainRendererDX12: Failed to create vertex buffer for chunk "
                          << chunkIndex << std::endl;
                continue;
            }

            // Upload vertex data
            void* vbData = m_chunkMeshes[chunkIndex].vertexBuffer->Map();
            if (vbData) {
                memcpy(vbData, vertices.data(), vbDesc.size);
                m_chunkMeshes[chunkIndex].vertexBuffer->Unmap();
            }

            // Create index buffer
            BufferDesc ibDesc;
            ibDesc.size = indices.size() * sizeof(uint32_t);
            ibDesc.usage = BufferUsage::Index;
            ibDesc.cpuAccess = true;
            ibDesc.debugName = "TerrainChunkIB";

            m_chunkMeshes[chunkIndex].indexBuffer = m_device->CreateBuffer(ibDesc);
            if (!m_chunkMeshes[chunkIndex].indexBuffer) {
                std::cerr << "TerrainRendererDX12: Failed to create index buffer for chunk "
                          << chunkIndex << std::endl;
                continue;
            }

            // Upload index data
            void* ibData = m_chunkMeshes[chunkIndex].indexBuffer->Map();
            if (ibData) {
                memcpy(ibData, indices.data(), ibDesc.size);
                m_chunkMeshes[chunkIndex].indexBuffer->Unmap();
            }

            m_chunkMeshes[chunkIndex].vertexCount = static_cast<uint32_t>(vertices.size());
            m_chunkMeshes[chunkIndex].indexCount = static_cast<uint32_t>(indices.size());
            m_chunkMeshes[chunkIndex].valid = true;

            // Calculate bounds
            glm::vec3 minBounds(FLT_MAX);
            glm::vec3 maxBounds(-FLT_MAX);
            for (const auto& v : vertices) {
                minBounds.x = std::min(minBounds.x, v.position[0]);
                minBounds.y = std::min(minBounds.y, v.position[1]);
                minBounds.z = std::min(minBounds.z, v.position[2]);
                maxBounds.x = std::max(maxBounds.x, v.position[0]);
                maxBounds.y = std::max(maxBounds.y, v.position[1]);
                maxBounds.z = std::max(maxBounds.z, v.position[2]);
            }
            m_chunkMeshes[chunkIndex].boundsMin = minBounds;
            m_chunkMeshes[chunkIndex].boundsMax = maxBounds;

            m_totalVertices += static_cast<int>(vertices.size());
            m_totalIndices += static_cast<int>(indices.size());
        }
    }

    return true;
}

// Helper to generate height at a world position using procedural noise
float TerrainRendererDX12::generateHeight(float worldX, float worldZ) const {
    using namespace TerrainRendererConfig;
    using namespace TerrainNoise;

    // Normalize coordinates for noise sampling
    float nx = worldX / static_cast<float>(WORLD_SIZE) + 0.5f;
    float nz = worldZ / static_cast<float>(WORLD_SIZE) + 0.5f;

    // Calculate distance from center for island falloff
    float centerX = 0.5f;
    float centerZ = 0.5f;
    float dx = nx - centerX;
    float dz = nz - centerZ;
    float distance = std::sqrt(dx * dx + dz * dz) * 2.0f; // 0 at center, ~1 at corners

    // Base continental shape - large scale features
    float continental = octaveNoise(nx * 2.0f, nz * 2.0f, 4, 0.6f);

    // Mountain ranges - medium scale with higher amplitude
    float mountains = octaveNoise(nx * 4.0f + 100.0f, nz * 4.0f + 100.0f, 6, 0.5f);
    mountains = std::pow(mountains, 1.5f); // Make peaks more dramatic

    // Hills and valleys - fine detail
    float hills = octaveNoise(nx * 8.0f + 50.0f, nz * 8.0f + 50.0f, 4, 0.5f);

    // Ridge lines for mountain chains (creates sharp ridges)
    float ridgeNoise = octaveNoise(nx * 3.0f + 200.0f, nz * 3.0f + 200.0f, 4, 0.5f);
    float ridges = 1.0f - std::abs(ridgeNoise * 2.0f - 1.0f);
    ridges = std::pow(ridges, 2.0f) * 0.3f;

    // Combine layers with weights
    float height = continental * 0.3f + mountains * 0.45f + hills * 0.15f + ridges;

    // Create distinct elevation zones
    // Low areas become water/beaches, high areas become mountains
    if (height < 0.35f) {
        // Flatten water areas slightly
        height = height * 0.8f;
    } else if (height > 0.7f) {
        // Exaggerate mountain peaks
        float excess = (height - 0.7f) / 0.3f;
        height = 0.7f + excess * excess * 0.3f;
    }

    // Apply island factor (falloff at edges) - gentler falloff
    float islandFactor = 1.0f - smoothstep(0.4f, 0.95f, distance);
    height = height * islandFactor;

    // Ensure we have both water (low values) and mountains (high values)
    // Shift the range slightly to ensure variety
    height = height * 1.1f - 0.05f;

    return std::max(0.0f, std::min(1.0f, height));
}

bool TerrainRendererDX12::generateChunkMesh(
    int chunkX, int chunkZ,
    std::vector<TerrainVertexDX12>& vertices,
    std::vector<uint32_t>& indices)
{
    using namespace TerrainRendererConfig;

    // Use simplified resolution for initial implementation (17 vertices per edge)
    const int resolution = 17;
    const float chunkSize = static_cast<float>(CHUNK_SIZE);
    const float heightScale = HEIGHT_SCALE;

    // Calculate world offset for this chunk
    // World goes from -WORLD_SIZE/2 to +WORLD_SIZE/2
    float worldOffsetX = static_cast<float>(chunkX * CHUNK_SIZE) -
                         static_cast<float>(WORLD_SIZE) / 2.0f;
    float worldOffsetZ = static_cast<float>(chunkZ * CHUNK_SIZE) -
                         static_cast<float>(WORLD_SIZE) / 2.0f;

    const float step = chunkSize / static_cast<float>(resolution - 1);

    // Lambda to get height at a position using procedural generation
    auto getHeight = [this, heightScale](float wx, float wz) -> float {
        return generateHeight(wx, wz) * heightScale;
    };

    // Generate vertices
    vertices.reserve(resolution * resolution);

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            TerrainVertexDX12 vertex;

            // Local position within chunk
            float localX = static_cast<float>(x) * step;
            float localZ = static_cast<float>(z) * step;

            // World position
            float worldX = worldOffsetX + localX;
            float worldZ = worldOffsetZ + localZ;

            // Get height
            float worldY = getHeight(worldX, worldZ);
            float normalizedHeight = worldY / heightScale;
            normalizedHeight = std::max(0.0f, std::min(1.0f, normalizedHeight));

            vertex.position[0] = worldX;
            vertex.position[1] = worldY;
            vertex.position[2] = worldZ;

            // Calculate normal using central differences
            float heightL = getHeight(worldX - step, worldZ);
            float heightR = getHeight(worldX + step, worldZ);
            float heightD = getHeight(worldX, worldZ - step);
            float heightU = getHeight(worldX, worldZ + step);

            glm::vec3 normal = glm::normalize(glm::vec3(
                heightL - heightR,
                2.0f * step,
                heightD - heightU
            ));

            vertex.normal[0] = normal.x;
            vertex.normal[1] = normal.y;
            vertex.normal[2] = normal.z;

            // Calculate slope (1.0 = flat, 0.0 = vertical cliff)
            float slope = 1.0f - normal.y;

            // Get biome color
            glm::vec3 color = getBiomeColor(normalizedHeight, slope);
            vertex.color[0] = color.r;
            vertex.color[1] = color.g;
            vertex.color[2] = color.b;

            // Texture coordinates (tiled across chunk)
            vertex.texCoord[0] = static_cast<float>(x) / static_cast<float>(resolution - 1);
            vertex.texCoord[1] = static_cast<float>(z) / static_cast<float>(resolution - 1);

            vertices.push_back(vertex);
        }
    }

    // Generate indices (triangle list)
    indices.reserve((resolution - 1) * (resolution - 1) * 6);

    for (int z = 0; z < resolution - 1; ++z) {
        for (int x = 0; x < resolution - 1; ++x) {
            uint32_t topLeft = z * resolution + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * resolution + x;
            uint32_t bottomRight = bottomLeft + 1;

            // First triangle (top-left, bottom-left, top-right)
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle (top-right, bottom-left, bottom-right)
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return true;
}

glm::vec3 TerrainRendererDX12::getBiomeColor(float height, float slope) const {
    using namespace BiomeConfig;

    // Slope factor (steeper = more rock)
    float rockBlend = std::min(1.0f, std::max(0.0f, slope * 3.0f));

    // Height-based color selection with smooth transitions
    glm::vec3 baseColor;

    if (height < WATER_LEVEL) {
        baseColor = WATER_COLOR;
    }
    else if (height < BEACH_LEVEL) {
        float t = (height - WATER_LEVEL) / (BEACH_LEVEL - WATER_LEVEL);
        baseColor = glm::mix(WATER_COLOR, BEACH_COLOR, t);
    }
    else if (height < GRASS_LEVEL) {
        float t = (height - BEACH_LEVEL) / (GRASS_LEVEL - BEACH_LEVEL);
        baseColor = glm::mix(BEACH_COLOR, GRASS_COLOR, t);
    }
    else if (height < FOREST_LEVEL) {
        float t = (height - GRASS_LEVEL) / (FOREST_LEVEL - GRASS_LEVEL);
        baseColor = glm::mix(GRASS_COLOR, FOREST_COLOR, t);
    }
    else if (height < SNOW_LEVEL) {
        float t = (height - FOREST_LEVEL) / (SNOW_LEVEL - FOREST_LEVEL);
        baseColor = glm::mix(FOREST_COLOR, MOUNTAIN_COLOR, t);
    }
    else {
        float t = std::min(1.0f, (height - SNOW_LEVEL) / (1.0f - SNOW_LEVEL));
        baseColor = glm::mix(MOUNTAIN_COLOR, SNOW_COLOR, t);
    }

    // Blend with rock color on steep slopes
    return glm::mix(baseColor, MOUNTAIN_COLOR, rockBlend);
}

void TerrainRendererDX12::update(const Camera& camera) {
    // Reserved for future LOD updates and chunk streaming
    // Currently all chunks are pre-generated at initialization
}

void TerrainRendererDX12::updateConstants(
    const glm::mat4& viewProj,
    const glm::mat4& world,
    const glm::vec3& cameraPos,
    const glm::vec3& lightDir,
    const glm::vec3& lightColor,
    float time)
{
    TerrainConstants constants;

    // Copy view-projection matrix
    memcpy(constants.viewProj, glm::value_ptr(viewProj), 16 * sizeof(float));

    // Copy world matrix
    memcpy(constants.world, glm::value_ptr(world), 16 * sizeof(float));

    // Camera position
    constants.cameraPos[0] = cameraPos.x;
    constants.cameraPos[1] = cameraPos.y;
    constants.cameraPos[2] = cameraPos.z;
    constants.cameraPos[3] = 0.0f;

    // Light direction
    constants.lightDir[0] = lightDir.x;
    constants.lightDir[1] = lightDir.y;
    constants.lightDir[2] = lightDir.z;
    constants.lightDir[3] = 0.0f;

    // Light color
    constants.lightColor[0] = lightColor.x;
    constants.lightColor[1] = lightColor.y;
    constants.lightColor[2] = lightColor.z;
    constants.lightColor[3] = 1.0f; // Intensity

    // Terrain scale parameters
    constants.terrainScale[0] = TerrainRendererConfig::HEIGHT_SCALE;
    constants.terrainScale[1] = static_cast<float>(TerrainRendererConfig::CHUNK_SIZE);
    constants.terrainScale[2] = TerrainRendererConfig::WATER_LEVEL;
    constants.terrainScale[3] = time;

    // Texture coordinate scale
    constants.texCoordScale[0] = 1.0f;
    constants.texCoordScale[1] = 1.0f;
    constants.texCoordScale[2] = 0.0f;
    constants.texCoordScale[3] = 0.0f;

    // Zero padding
    memset(constants.padding, 0, sizeof(constants.padding));

    // Upload to constant buffer
    void* cbData = m_constantBuffer->Map();
    if (cbData) {
        memcpy(cbData, &constants, sizeof(TerrainConstants));
        m_constantBuffer->Unmap();
    }
}

void TerrainRendererDX12::render(
    ICommandList* cmdList,
    IPipeline* pipeline,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const glm::vec3& cameraPos,
    const glm::vec3& lightDir,
    const glm::vec3& lightColor,
    float time)
{
    if (!m_initialized || !cmdList || !pipeline) {
        return;
    }

    // Build view-projection matrix
    glm::mat4 viewProj = projMatrix * viewMatrix;

    // Get frustum for culling
    Frustum frustum;
    frustum.update(viewProj);

    // Set pipeline
    cmdList->SetPipeline(pipeline);

    m_renderedChunks = 0;
    m_culledChunks = 0;

    // Identity world matrix (vertices are already in world space)
    glm::mat4 world = glm::mat4(1.0f);

    // Update constant buffer
    updateConstants(viewProj, world, cameraPos, lightDir, lightColor, time);

    // Bind constant buffer
    cmdList->BindConstantBuffer(0, m_constantBuffer.get(), 0);

    // Render each visible chunk
    for (size_t i = 0; i < m_chunkMeshes.size(); ++i) {
        const auto& mesh = m_chunkMeshes[i];

        if (!mesh.valid || !mesh.vertexBuffer || !mesh.indexBuffer) {
            continue;
        }

        // Frustum culling using AABB
        glm::vec3 center = (mesh.boundsMin + mesh.boundsMax) * 0.5f;
        glm::vec3 extents = (mesh.boundsMax - mesh.boundsMin) * 0.5f;
        float radius = glm::length(extents);

        if (!frustum.isSphereVisible(center, radius)) {
            m_culledChunks++;
            continue;
        }

        // Bind vertex and index buffers
        cmdList->BindVertexBuffer(0, mesh.vertexBuffer.get(), sizeof(TerrainVertexDX12), 0);
        cmdList->BindIndexBuffer(mesh.indexBuffer.get(), IndexFormat::UInt32, 0);

        // Draw
        cmdList->DrawIndexed(mesh.indexCount, 0, 0);

        m_renderedChunks++;
    }
}

void TerrainRendererDX12::renderForShadow(
    ICommandList* cmdList,
    IPipeline* shadowPipeline,
    const glm::mat4& lightViewProj)
{
    if (!m_initialized || !cmdList || !shadowPipeline) {
        return;
    }

    // Set shadow pipeline
    cmdList->SetPipeline(shadowPipeline);

    // Update constants with light view-projection
    glm::mat4 world = glm::mat4(1.0f);
    glm::vec3 lightDir(0.5f, -0.8f, 0.3f);  // Default for shadow pass
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    updateConstants(lightViewProj, world, glm::vec3(0.0f), lightDir, lightColor, 0.0f);

    // Bind constant buffer
    cmdList->BindConstantBuffer(0, m_constantBuffer.get(), 0);

    // Render all chunks (no frustum culling for shadow maps - they cover large area)
    for (size_t i = 0; i < m_chunkMeshes.size(); ++i) {
        const auto& mesh = m_chunkMeshes[i];

        if (!mesh.valid || !mesh.vertexBuffer || !mesh.indexBuffer) {
            continue;
        }

        // Bind vertex and index buffers
        cmdList->BindVertexBuffer(0, mesh.vertexBuffer.get(), sizeof(TerrainVertexDX12), 0);
        cmdList->BindIndexBuffer(mesh.indexBuffer.get(), IndexFormat::UInt32, 0);

        // Draw
        cmdList->DrawIndexed(mesh.indexCount, 0, 0);
    }
}
