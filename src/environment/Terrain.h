#pragma once

// DirectX 12 headers
#include <d3d12.h>
#include <wrl/client.h>

#include <glm/glm.hpp>
#include <vector>

// Include the actual DX12Device header to avoid forward declaration conflicts
#include "../graphics/DX12Device.h"
struct ID3D12PipelineState;
struct ID3D12RootSignature;

using Microsoft::WRL::ComPtr;

// Terrain vertex format (must match HLSL VSInput)
struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Terrain {
public:
    Terrain(int width, int depth, float scale = 1.0f);
    ~Terrain();

    // Initialize DX12 resources (must be called after DX12Device is ready)
    void initializeDX12(DX12Device* device, ID3D12PipelineState* pso, ID3D12RootSignature* rootSig);

    void generate(unsigned int seed);

    // Render terrain using the provided command list
    // Caller must have already set the PSO and root signature
    void render(ID3D12GraphicsCommandList* commandList);

    // Render geometry only for shadow pass (caller sets shadow PSO)
    void renderForShadow(ID3D12GraphicsCommandList* commandList);

    /**
     * Get terrain height at world coordinates.
     * Returns 0.0f for out-of-bounds coordinates (graceful fallback).
     */
    float getHeight(float x, float z) const;

    /**
     * Get terrain height with bounds checking.
     */
    bool getHeightSafe(float x, float z, float& outHeight) const;

    /**
     * Check if world coordinates are within terrain bounds.
     */
    bool isInBounds(float x, float z) const;

    /**
     * Clamp world coordinates to terrain bounds.
     */
    void clampToBounds(float& x, float& z) const;

    // Accessors
    unsigned int getIndexCount() const { return indexCount; }
    bool isWater(float x, float z) const;

    int getWidth() const { return width; }
    int getDepth() const { return depth; }
    float getScale() const { return scale; }
    float getWaterLevel() const { return waterLevel; }

    /**
     * Get terrain normal at world coordinates.
     * Computes normal from surrounding height samples.
     */
    glm::vec3 getNormal(float x, float z) const;

    // DX12 buffer views (for advanced use cases)
    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const { return vertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const { return indexBufferView; }

private:
    int width;
    int depth;
    float scale;
    float waterLevel;

    std::vector<float> heightMap;
    unsigned int indexCount;

    // DX12 resources
    DX12Device* dx12Device = nullptr;
    ID3D12PipelineState* terrainPSO = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;

    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> vertexUploadBuffer;  // For initial upload
    ComPtr<ID3D12Resource> indexUploadBuffer;   // For initial upload

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

    void setupMesh();
    void createBuffers(const std::vector<TerrainVertex>& vertices,
                       const std::vector<unsigned int>& indices);
    glm::vec3 getTerrainColor(float height);
};
