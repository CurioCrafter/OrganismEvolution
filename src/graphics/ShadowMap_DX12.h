#pragma once

// DirectX 12 Shadow Map Implementation
// Supports both single shadow map and Cascaded Shadow Maps (CSM)
// Creates depth-only render target for shadow mapping with PCF support

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif

#include <DirectXMath.h>
#include <cstdint>
#include <iostream>
#include <array>

// Shadow map configuration
static constexpr uint32_t SHADOW_MAP_SIZE_DX12 = 2048;
static constexpr uint32_t CSM_CASCADE_COUNT = 4;

// ============================================================================
// Cascaded Shadow Map (CSM) Data Structure
// Used for GPU constant buffer upload
// ============================================================================
struct CSMConstants {
    DirectX::XMFLOAT4X4 cascadeViewProj[CSM_CASCADE_COUNT];  // Light space matrices per cascade
    DirectX::XMFLOAT4 cascadeSplits;                          // View-space depth splits
    DirectX::XMFLOAT4 cascadeOffsets[CSM_CASCADE_COUNT];      // UV offsets for atlas (if using atlas)
    DirectX::XMFLOAT4 cascadeScales[CSM_CASCADE_COUNT];       // UV scales for atlas (if using atlas)
};

// ============================================================================
// Legacy Single Shadow Map (kept for compatibility)
// ============================================================================
class ShadowMapDX12 {
public:
    ShadowMapDX12();
    ~ShadowMapDX12();

    // Initialize shadow map resources
    bool init(ID3D12Device* device,
              ID3D12DescriptorHeap* dsvHeap,
              ID3D12DescriptorHeap* srvHeap,
              uint32_t dsvHeapIndex,
              uint32_t srvHeapIndex,
              uint32_t width = SHADOW_MAP_SIZE_DX12,
              uint32_t height = SHADOW_MAP_SIZE_DX12);

    void cleanup();
    void beginShadowPass(ID3D12GraphicsCommandList* cmdList);
    void endShadowPass(ID3D12GraphicsCommandList* cmdList);

    D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle() const { return m_dsvHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE getSRVGpuHandle() const { return m_srvGpuHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE getSRVCpuHandle() const { return m_srvCpuHandle; }

    void updateLightSpaceMatrix(const DirectX::XMFLOAT3& lightDir,
                                 const DirectX::XMFLOAT3& sceneCenter,
                                 float sceneRadius);

    const DirectX::XMMATRIX& getLightSpaceMatrix() const { return m_lightSpaceMatrix; }
    DirectX::XMFLOAT4X4 getLightSpaceMatrixFloat4x4() const;

    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    const D3D12_VIEWPORT& getViewport() const { return m_viewport; }
    const D3D12_RECT& getScissorRect() const { return m_scissorRect; }
    ID3D12Resource* getDepthResource() const { return m_depthTexture.Get(); }
    bool isInitialized() const { return m_initialized; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle;
    uint32_t m_width;
    uint32_t m_height;
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    DirectX::XMMATRIX m_lightView;
    DirectX::XMMATRIX m_lightProjection;
    DirectX::XMMATRIX m_lightSpaceMatrix;
    D3D12_RESOURCE_STATES m_currentState;
    bool m_initialized;
};

// ============================================================================
// Cascaded Shadow Map Implementation
// Provides high-quality shadows across large view distances
// ============================================================================
class CascadedShadowMapDX12 {
public:
    CascadedShadowMapDX12();
    ~CascadedShadowMapDX12();

    // Initialize CSM resources
    // Creates a Texture2DArray with CASCADE_COUNT slices
    // device: D3D12 device for resource creation
    // dsvHeap: Descriptor heap for DSV (needs CASCADE_COUNT slots starting at dsvHeapStartIndex)
    // srvHeap: Descriptor heap for SRV (needs 1 slot for array SRV)
    // dsvHeapStartIndex: Starting index in DSV heap for cascade DSVs
    // srvHeapIndex: Index in SRV heap for the shadow map array SRV
    bool init(ID3D12Device* device,
              ID3D12DescriptorHeap* dsvHeap,
              ID3D12DescriptorHeap* srvHeap,
              uint32_t dsvHeapStartIndex,
              uint32_t srvHeapIndex,
              uint32_t cascadeSize = SHADOW_MAP_SIZE_DX12);

    void cleanup();

    // Update cascade frustums based on camera and light direction
    // camera matrices should be the current frame's view/projection
    // lightDir: Normalized direction TO the light (sun direction)
    // nearPlane/farPlane: Camera's near/far clip planes
    void updateCascades(const DirectX::XMMATRIX& cameraView,
                        const DirectX::XMMATRIX& cameraProjection,
                        const DirectX::XMFLOAT3& lightDir,
                        float nearPlane,
                        float farPlane);

    // Begin rendering a specific cascade
    // Sets viewport, transitions to depth write, clears depth
    void beginCascade(uint32_t cascadeIndex, ID3D12GraphicsCommandList* cmdList);

    // End rendering current cascade (no transition yet - batched)
    void endCascade(ID3D12GraphicsCommandList* cmdList);

    // Transition all cascades to shader resource state after all cascades rendered
    void endShadowPass(ID3D12GraphicsCommandList* cmdList);

    // Get light space matrix for specific cascade (for rendering)
    const DirectX::XMMATRIX& getCascadeViewProj(uint32_t cascadeIndex) const;
    DirectX::XMFLOAT4X4 getCascadeViewProjFloat4x4(uint32_t cascadeIndex) const;

    // Get CSM constants for shader upload
    CSMConstants getCSMConstants() const;

    // Get cascade split distances (view-space depths)
    const std::array<float, CSM_CASCADE_COUNT>& getCascadeSplits() const { return m_cascadeSplits; }

    // Set custom cascade split distances
    void setCascadeSplits(const std::array<float, CSM_CASCADE_COUNT>& splits);

    // Get SRV for shader binding (Texture2DArray)
    D3D12_GPU_DESCRIPTOR_HANDLE getSRVGpuHandle() const { return m_srvGpuHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE getSRVCpuHandle() const { return m_srvCpuHandle; }

    // Get DSV for specific cascade
    D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle(uint32_t cascadeIndex) const;

    // Get cascade dimensions
    uint32_t getCascadeSize() const { return m_cascadeSize; }
    uint32_t getCascadeCount() const { return CSM_CASCADE_COUNT; }

    // Get viewport/scissor for shadow pass
    const D3D12_VIEWPORT& getViewport() const { return m_viewport; }
    const D3D12_RECT& getScissorRect() const { return m_scissorRect; }

    // Get depth resource for debugging
    ID3D12Resource* getDepthResource() const { return m_shadowMapArray.Get(); }

    bool isInitialized() const { return m_initialized; }

    // Debug: Enable/disable cascade debug visualization
    bool debugVisualizeCascades = false;

private:
    // Calculate frustum corners in world space for a given depth range
    void calculateFrustumCorners(const DirectX::XMMATRIX& invViewProj,
                                  float nearSplit, float farSplit,
                                  DirectX::XMFLOAT3 corners[8]) const;

    // Calculate tight orthographic bounds from frustum corners
    void calculateCascadeBounds(const DirectX::XMFLOAT3 corners[8],
                                 const DirectX::XMFLOAT3& lightDir,
                                 DirectX::XMMATRIX& lightViewProj,
                                 float& nearZ, float& farZ) const;

    // D3D12 Resources
    Microsoft::WRL::ComPtr<ID3D12Resource> m_shadowMapArray;  // Texture2DArray

    // Per-cascade DSV handles
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, CSM_CASCADE_COUNT> m_dsvHandles;

    // Single SRV for the entire array
    D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle;

    // Per-cascade light matrices
    std::array<DirectX::XMMATRIX, CSM_CASCADE_COUNT> m_cascadeViewProj;

    // Cascade split depths (in view space)
    // Default: practical split scheme optimized for typical game scenarios
    std::array<float, CSM_CASCADE_COUNT> m_cascadeSplits = {15.0f, 50.0f, 150.0f, 500.0f};

    // Shadow map dimensions (same for all cascades)
    uint32_t m_cascadeSize;

    // Shared viewport and scissor rect
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    // Resource state tracking
    D3D12_RESOURCE_STATES m_currentState;
    uint32_t m_currentCascade;  // Currently rendering cascade

    // Device reference for descriptor size calculations
    ID3D12Device* m_device;
    uint32_t m_dsvDescriptorSize;

    bool m_initialized;
};
