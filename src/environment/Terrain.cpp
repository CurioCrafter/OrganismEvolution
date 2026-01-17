#include "Terrain.h"
#ifndef USE_FORGE_ENGINE
// Only include DX12Device when not using Forge Engine
// (Forge Engine has its own device abstraction)
#include "../graphics/DX12Device.h"
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

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

        return (total / maxValue + 1.0f) * 0.5f;
    }

    inline float smoothstep(float edge0, float edge1, float x) {
        float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    }

    inline float sampleHeightNormalized(float worldX, float worldZ) {
        constexpr float kWorldSize = 2048.0f;

        float nx = worldX / kWorldSize + 0.5f;
        float nz = worldZ / kWorldSize + 0.5f;

        float dx = nx - 0.5f;
        float dz = nz - 0.5f;
        float distance = std::sqrt(dx * dx + dz * dz) * 2.0f;

        float continental = octaveNoise(nx * 2.0f, nz * 2.0f, 4, 0.6f);
        float mountains = octaveNoise(nx * 4.0f + 100.0f, nz * 4.0f + 100.0f, 6, 0.5f);
        mountains = std::pow(mountains, 1.5f);
        float hills = octaveNoise(nx * 8.0f + 50.0f, nz * 8.0f + 50.0f, 4, 0.5f);

        float ridgeNoise = octaveNoise(nx * 3.0f + 200.0f, nz * 3.0f + 200.0f, 4, 0.5f);
        float ridges = 1.0f - std::abs(ridgeNoise * 2.0f - 1.0f);
        ridges = std::pow(ridges, 2.0f) * 0.3f;

        float height = continental * 0.3f + mountains * 0.45f + hills * 0.15f + ridges;

        if (height < 0.35f) {
            height = height * 0.8f;
        } else if (height > 0.7f) {
            float excess = (height - 0.7f) / 0.3f;
            height = 0.7f + excess * excess * 0.3f;
        }

        float islandFactor = 1.0f - smoothstep(0.4f, 0.95f, distance);
        height = height * islandFactor;
        height = height * 1.1f - 0.05f;

        return std::max(0.0f, std::min(1.0f, height));
    }
}

// Helper function
float smoothstep(float edge0, float edge1, float x);

Terrain::Terrain(int width, int depth, float scale)
    : width(width), depth(depth), scale(scale), waterLevel(0.35f), indexCount(0) {
}

Terrain::~Terrain() {
    // ComPtr handles Release() automatically
    // No explicit cleanup needed for DX12 resources
}

void Terrain::initializeDX12(DX12Device* device, ID3D12PipelineState* pso, ID3D12RootSignature* rootSig) {
    dx12Device = device;
    terrainPSO = pso;
    rootSignature = rootSig;
}

void Terrain::generate(unsigned int seed) {
    heightMap.resize(width * depth);

    // Generate height map using the shared terrain noise profile
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            float worldX = (x - width / 2.0f) * scale;
            float worldZ = (z - depth / 2.0f) * scale;
            float height = TerrainNoise::sampleHeightNormalized(worldX, worldZ);

            heightMap[z * width + x] = height;
        }
    }

    setupMesh();
}

void Terrain::setupMesh() {
    std::vector<TerrainVertex> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            float height = heightMap[z * width + x];

            TerrainVertex vertex;
            vertex.position = glm::vec3(
                (x - width / 2.0f) * scale,
                height * 30.0f, // Height scale
                (z - depth / 2.0f) * scale
            );

            vertex.color = getTerrainColor(height);

            // Calculate normal (simplified - average of adjacent heights)
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            if (x > 0 && x < width - 1 && z > 0 && z < depth - 1) {
                float hL = heightMap[z * width + (x - 1)];
                float hR = heightMap[z * width + (x + 1)];
                float hD = heightMap[(z - 1) * width + x];
                float hU = heightMap[(z + 1) * width + x];

                vertex.normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));
            }

            vertices.push_back(vertex);
        }
    }

    // Validate terrain vertex data for corruption
    bool hasNaN = false;
    bool hasZeroNormal = false;
    for (const auto& v : vertices) {
        if (std::isnan(v.position.x) || std::isnan(v.position.y) || std::isnan(v.position.z) ||
            std::isnan(v.normal.x) || std::isnan(v.normal.y) || std::isnan(v.normal.z) ||
            std::isnan(v.color.r) || std::isnan(v.color.g) || std::isnan(v.color.b)) {
            hasNaN = true;
            break;
        }
        // Check for zero-length normals (except edge vertices which have default normal)
        float normalLen = v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z;
        if (normalLen < 0.001f) {
            hasZeroNormal = true;
        }
    }
    if (hasNaN) {
        std::cerr << "WARNING: Terrain has NaN values in vertex data!" << std::endl;
    }
    if (hasZeroNormal) {
        std::cerr << "WARNING: Terrain has zero-length normals!" << std::endl;
    }

    // Print height range for debugging
    if (!vertices.empty()) {
        float minY = vertices[0].position.y;
        float maxY = vertices[0].position.y;
        for (const auto& v : vertices) {
            minY = std::min(minY, v.position.y);
            maxY = std::max(maxY, v.position.y);
        }
        std::cout << "Terrain height range: " << minY << " to " << maxY << std::endl;
    }

    // Generate indices
    for (int z = 0; z < depth - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            unsigned int topLeft = z * width + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * width + x;
            unsigned int bottomRight = bottomLeft + 1;

            // Triangle 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Triangle 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = static_cast<unsigned int>(indices.size());

    std::cout << "Terrain mesh created: " << vertices.size() << " vertices, "
              << indexCount << " indices" << std::endl;

    // Debug: Print first vertex
    if (!vertices.empty()) {
        const auto& v = vertices[0];
        std::cout << "First vertex - Pos: (" << v.position.x << ", " << v.position.y << ", " << v.position.z
                  << ") Normal: (" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z
                  << ") Color: (" << v.color.r << ", " << v.color.g << ", " << v.color.b << ")" << std::endl;
    }

    // Debug: Print center vertex
    size_t centerIdx = vertices.size() / 2;
    if (centerIdx < vertices.size()) {
        const auto& v = vertices[centerIdx];
        std::cout << "Center vertex - Pos: (" << v.position.x << ", " << v.position.y << ", " << v.position.z
                  << ") Height: " << v.position.y
                  << " Color: (" << v.color.r << ", " << v.color.g << ", " << v.color.b << ")" << std::endl;
    }

    // Create DX12 buffers if device is available
    if (dx12Device) {
        createBuffers(vertices, indices);
    }
}

#ifndef USE_FORGE_ENGINE
// DX12Device-specific buffer creation - only used when not using Forge Engine
void Terrain::createBuffers(const std::vector<TerrainVertex>& vertices,
                            const std::vector<unsigned int>& indices) {
    if (!dx12Device) {
        std::cerr << "ERROR: DX12Device not initialized for terrain buffers" << std::endl;
        return;
    }

    ID3D12Device* device = dx12Device->GetDevice();
    if (!device) {
        std::cerr << "ERROR: Could not get ID3D12Device" << std::endl;
        return;
    }

    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(TerrainVertex));
    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(unsigned int));

    // Create vertex buffer (default heap for GPU-only access)
    D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Width = vertexBufferSize;
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexBufferDesc.SampleDesc.Count = 1;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer));

    if (FAILED(hr)) {
        std::cerr << "ERROR: Failed to create terrain vertex buffer (HRESULT: 0x"
                  << std::hex << hr << std::dec << ")" << std::endl;
        return;
    }

    // Create index buffer (default heap)
    D3D12_RESOURCE_DESC indexBufferDesc = vertexBufferDesc;
    indexBufferDesc.Width = indexBufferSize;

    hr = device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&indexBuffer));

    if (FAILED(hr)) {
        std::cerr << "ERROR: Failed to create terrain index buffer (HRESULT: 0x"
                  << std::hex << hr << std::dec << ")" << std::endl;
        return;
    }

    // Create upload buffers (upload heap for CPU write access)
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadBufferDesc = vertexBufferDesc;
    uploadBufferDesc.Width = vertexBufferSize;

    hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexUploadBuffer));

    if (FAILED(hr)) {
        std::cerr << "ERROR: Failed to create vertex upload buffer" << std::endl;
        return;
    }

    uploadBufferDesc.Width = indexBufferSize;
    hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexUploadBuffer));

    if (FAILED(hr)) {
        std::cerr << "ERROR: Failed to create index upload buffer" << std::endl;
        return;
    }

    // Copy vertex data to upload buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };  // We don't read from this resource

    hr = vertexUploadBuffer->Map(0, &readRange, &mappedData);
    if (SUCCEEDED(hr)) {
        memcpy(mappedData, vertices.data(), vertexBufferSize);
        vertexUploadBuffer->Unmap(0, nullptr);
    }

    // Copy index data to upload buffer
    hr = indexUploadBuffer->Map(0, &readRange, &mappedData);
    if (SUCCEEDED(hr)) {
        memcpy(mappedData, indices.data(), indexBufferSize);
        indexUploadBuffer->Unmap(0, nullptr);
    }

    // Use the command list from DX12Device to copy and transition
    ID3D12GraphicsCommandList* commandList = dx12Device->GetCommandList();
    if (commandList) {
        // Copy from upload to default heap
        commandList->CopyResource(vertexBuffer.Get(), vertexUploadBuffer.Get());
        commandList->CopyResource(indexBuffer.Get(), indexUploadBuffer.Get());

        // Transition to vertex/index buffer state
        D3D12_RESOURCE_BARRIER barriers[2] = {};

        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = vertexBuffer.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = indexBuffer.Get();
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(2, barriers);
    }

    // Setup buffer views
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = vertexBufferSize;
    vertexBufferView.StrideInBytes = sizeof(TerrainVertex);

    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = indexBufferSize;
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    std::cout << "Terrain DX12 buffers created: VB=" << vertexBufferSize
              << " bytes, IB=" << indexBufferSize << " bytes" << std::endl;
}
#else
// Stub for Forge Engine mode
void Terrain::createBuffers(const std::vector<TerrainVertex>& /*vertices*/,
                            const std::vector<unsigned int>& /*indices*/) {
    // Buffer creation handled by TerrainRendererDX12 when using Forge Engine
}
#endif

#ifndef USE_FORGE_ENGINE
void Terrain::render(ID3D12GraphicsCommandList* commandList) {
    if (!commandList || !vertexBuffer || !indexBuffer) {
        return;
    }

    // Bind vertex and index buffers
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw the entire terrain with a single indexed draw call
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void Terrain::renderForShadow(ID3D12GraphicsCommandList* commandList) {
    // Same geometry, caller should have set shadow PSO
    render(commandList);
}
#else
// Stubs for Forge Engine mode
void Terrain::render(ID3D12GraphicsCommandList* /*commandList*/) {
    // Rendering handled by TerrainRendererDX12 when using Forge Engine
}

void Terrain::renderForShadow(ID3D12GraphicsCommandList* /*commandList*/) {
    // Rendering handled by TerrainRendererDX12 when using Forge Engine
}
#endif

float Terrain::getHeight(float x, float z) const {
    if (heightMap.empty() || width <= 1 || depth <= 1) {
        return 0.0f;
    }

    // Convert world coordinates to grid coordinates
    float gridXf = (x / scale) + width / 2.0f;
    float gridZf = (z / scale) + depth / 2.0f;

    // Bounds check with graceful fallback
    if (gridXf < 0.0f || gridXf > static_cast<float>(width - 1) ||
        gridZf < 0.0f || gridZf > static_cast<float>(depth - 1)) {
        return 0.0f;
    }

    int x0 = static_cast<int>(std::floor(gridXf));
    int z0 = static_cast<int>(std::floor(gridZf));
    int x1 = std::min(x0 + 1, width - 1);
    int z1 = std::min(z0 + 1, depth - 1);

    float tx = gridXf - static_cast<float>(x0);
    float tz = gridZf - static_cast<float>(z0);

    auto sample = [this](int gx, int gz) -> float {
        size_t index = static_cast<size_t>(gz * width + gx);
        if (index >= heightMap.size()) {
            return 0.0f;
        }
        return heightMap[index];
    };

    float h00 = sample(x0, z0);
    float h10 = sample(x1, z0);
    float h01 = sample(x0, z1);
    float h11 = sample(x1, z1);

    float h0 = h00 + (h10 - h00) * tx;
    float h1 = h01 + (h11 - h01) * tx;
    float h = h0 + (h1 - h0) * tz;

    return h * 30.0f;
}

bool Terrain::getHeightSafe(float x, float z, float& outHeight) const {
    if (heightMap.empty() || width <= 1 || depth <= 1) {
        return false;
    }

    float gridXf = (x / scale) + width / 2.0f;
    float gridZf = (z / scale) + depth / 2.0f;

    if (gridXf < 0.0f || gridXf > static_cast<float>(width - 1) ||
        gridZf < 0.0f || gridZf > static_cast<float>(depth - 1)) {
        return false;
    }

    int x0 = static_cast<int>(std::floor(gridXf));
    int z0 = static_cast<int>(std::floor(gridZf));
    int x1 = std::min(x0 + 1, width - 1);
    int z1 = std::min(z0 + 1, depth - 1);

    float tx = gridXf - static_cast<float>(x0);
    float tz = gridZf - static_cast<float>(z0);

    auto sample = [this](int gx, int gz) -> float {
        size_t index = static_cast<size_t>(gz * width + gx);
        if (index >= heightMap.size()) {
            return 0.0f;
        }
        return heightMap[index];
    };

    float h00 = sample(x0, z0);
    float h10 = sample(x1, z0);
    float h01 = sample(x0, z1);
    float h11 = sample(x1, z1);

    float h0 = h00 + (h10 - h00) * tx;
    float h1 = h01 + (h11 - h01) * tx;
    float h = h0 + (h1 - h0) * tz;

    outHeight = h * 30.0f;
    return true;
}

bool Terrain::isInBounds(float x, float z) const {
    float gridXf = (x / scale) + width / 2.0f;
    float gridZf = (z / scale) + depth / 2.0f;

    int gridX = static_cast<int>(gridXf);
    int gridZ = static_cast<int>(gridZf);

    return gridX >= 0 && gridX < width && gridZ >= 0 && gridZ < depth;
}

void Terrain::clampToBounds(float& x, float& z) const {
    float minX = -width / 2.0f * scale;
    float maxX = (width / 2.0f - 1) * scale;
    float minZ = -depth / 2.0f * scale;
    float maxZ = (depth / 2.0f - 1) * scale;

    x = std::max(minX, std::min(x, maxX));
    z = std::max(minZ, std::min(z, maxZ));
}

bool Terrain::isWater(float x, float z) const {
    float height = 0.0f;
    if (!getHeightSafe(x, z, height)) {
        return true;
    }

    return height < waterLevel * 30.0f;
}

glm::vec3 Terrain::getNormal(float x, float z) const {
    // Compute normal from surrounding height samples using central differences
    float eps = scale * 0.5f;  // Half grid step
    float hL = getHeight(x - eps, z);
    float hR = getHeight(x + eps, z);
    float hD = getHeight(x, z - eps);
    float hU = getHeight(x, z + eps);

    // Tangent vectors
    glm::vec3 tangentX(eps * 2.0f, hR - hL, 0.0f);
    glm::vec3 tangentZ(0.0f, hU - hD, eps * 2.0f);

    // Normal is cross product of tangents
    glm::vec3 normal = glm::normalize(glm::cross(tangentZ, tangentX));
    return normal;
}

glm::vec3 Terrain::getTerrainColor(float height) {
    // Water
    if (height < waterLevel) {
        return glm::vec3(0.2f, 0.4f, 0.8f);
    }
    // Beach/Sand
    else if (height < 0.42f) {
        return glm::vec3(0.9f, 0.85f, 0.6f);
    }
    // Grass
    else if (height < 0.65f) {
        return glm::vec3(0.3f, 0.7f, 0.3f);
    }
    // Forest
    else if (height < 0.8f) {
        return glm::vec3(0.2f, 0.5f, 0.2f);
    }
    // Mountain
    else {
        return glm::vec3(0.6f, 0.6f, 0.6f);
    }
}

float smoothstep(float edge0, float edge1, float x) {
    float t = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
