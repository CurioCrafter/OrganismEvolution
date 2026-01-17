#include "ShadowMap_DX12.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ShadowMapDX12::ShadowMapDX12()
    : m_width(0)
    , m_height(0)
    , m_dsvHandle{}
    , m_srvCpuHandle{}
    , m_srvGpuHandle{}
    , m_viewport{}
    , m_scissorRect{}
    , m_lightView(XMMatrixIdentity())
    , m_lightProjection(XMMatrixIdentity())
    , m_lightSpaceMatrix(XMMatrixIdentity())
    , m_currentState(D3D12_RESOURCE_STATE_COMMON)
    , m_initialized(false)
{
}

ShadowMapDX12::~ShadowMapDX12()
{
    cleanup();
}

bool ShadowMapDX12::init(ID3D12Device* device,
                          ID3D12DescriptorHeap* dsvHeap,
                          ID3D12DescriptorHeap* srvHeap,
                          uint32_t dsvHeapIndex,
                          uint32_t srvHeapIndex,
                          uint32_t width,
                          uint32_t height)
{
    if (m_initialized) {
        cleanup();
    }

    if (!device || !dsvHeap || !srvHeap) {
        std::cerr << "[ShadowMapDX12] Invalid device or descriptor heaps!" << std::endl;
        return false;
    }

    m_width = width;
    m_height = height;

    // ========================================================================
    // Create depth texture resource (ID3D12Resource)
    // ========================================================================

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    // Use TYPELESS format to allow both DSV (D32_FLOAT) and SRV (R32_FLOAT) views
    depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    // Allow both depth stencil and shader resource usage
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Optimized clear value for depth
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // Create resource in default heap
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // Start in DEPTH_WRITE state for initial shadow pass
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_depthTexture)
    );

    if (FAILED(hr)) {
        std::cerr << "[ShadowMapDX12] Failed to create depth texture! HRESULT: "
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    m_depthTexture->SetName(L"ShadowMap_DepthTexture");
    m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    // ========================================================================
    // Create Depth Stencil View (DSV) for shadow pass rendering
    // ========================================================================

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    // Calculate DSV handle from heap
    uint32_t dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_dsvHandle.ptr += static_cast<SIZE_T>(dsvHeapIndex) * dsvDescriptorSize;

    device->CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc, m_dsvHandle);

    // ========================================================================
    // Create Shader Resource View (SRV) for sampling in main pass
    // ========================================================================

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;  // Read as R32_FLOAT for shadow comparison
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // Calculate SRV handles from heap
    uint32_t srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_srvCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvCpuHandle.ptr += static_cast<SIZE_T>(srvHeapIndex) * srvDescriptorSize;

    m_srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    m_srvGpuHandle.ptr += static_cast<SIZE_T>(srvHeapIndex) * srvDescriptorSize;

    device->CreateShaderResourceView(m_depthTexture.Get(), &srvDesc, m_srvCpuHandle);

    // ========================================================================
    // Setup viewport and scissor rect for shadow pass
    // ========================================================================

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(width);
    m_scissorRect.bottom = static_cast<LONG>(height);

    m_initialized = true;
    std::cout << "[ShadowMapDX12] Initialized " << width << "x" << height
              << " shadow map (D32_FLOAT)" << std::endl;

    return true;
}

void ShadowMapDX12::cleanup()
{
    m_depthTexture.Reset();
    m_dsvHandle = {};
    m_srvCpuHandle = {};
    m_srvGpuHandle = {};
    m_initialized = false;
}

void ShadowMapDX12::beginShadowPass(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList) {
        return;
    }

    // Transition from shader resource (read) to depth write state if needed
    if (m_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_depthTexture.Get();
        barrier.Transition.StateBefore = m_currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    // Clear depth buffer to 1.0 (far plane)
    cmdList->ClearDepthStencilView(m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set viewport and scissor for shadow map resolution
    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissorRect);

    // Set depth-only render target (no color target)
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &m_dsvHandle);
}

void ShadowMapDX12::endShadowPass(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList) {
        return;
    }

    // Transition from depth write to pixel shader resource for sampling
    if (m_currentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_depthTexture.Get();
        barrier.Transition.StateBefore = m_currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        m_currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

void ShadowMapDX12::updateLightSpaceMatrix(const XMFLOAT3& lightDir,
                                            const XMFLOAT3& sceneCenter,
                                            float sceneRadius)
{
    // Normalize light direction
    XMVECTOR lightDirection = XMVector3Normalize(XMLoadFloat3(&lightDir));

    // Position the light far along the light direction from scene center
    XMVECTOR center = XMLoadFloat3(&sceneCenter);
    XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(lightDirection, sceneRadius * 2.0f));

    // Create light view matrix (looking at scene center from light position)
    // Use (0,1,0) as up vector, but handle case when light is directly overhead
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // Check if light is nearly parallel to up vector
    float dotResult;
    XMStoreFloat(&dotResult, XMVector3Dot(lightDirection, up));
    if (fabsf(dotResult) > 0.99f) {
        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    m_lightView = XMMatrixLookAtLH(lightPos, center, up);

    // Create orthographic projection that covers the entire scene
    // Add some padding to prevent shadow clipping
    float orthoSize = sceneRadius * 1.5f;
    float nearPlane = 0.1f;
    float farPlane = sceneRadius * 4.0f;

    m_lightProjection = XMMatrixOrthographicLH(
        orthoSize * 2.0f,  // width
        orthoSize * 2.0f,  // height
        nearPlane,
        farPlane
    );

    // Combine into light space matrix (view * projection for column-major)
    m_lightSpaceMatrix = XMMatrixMultiply(m_lightView, m_lightProjection);
}

XMFLOAT4X4 ShadowMapDX12::getLightSpaceMatrixFloat4x4() const
{
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, m_lightSpaceMatrix);
    return result;
}

// ============================================================================
// CascadedShadowMapDX12 Implementation
// ============================================================================

CascadedShadowMapDX12::CascadedShadowMapDX12()
    : m_cascadeSize(0)
    , m_srvCpuHandle{}
    , m_srvGpuHandle{}
    , m_viewport{}
    , m_scissorRect{}
    , m_currentState(D3D12_RESOURCE_STATE_COMMON)
    , m_currentCascade(0)
    , m_device(nullptr)
    , m_dsvDescriptorSize(0)
    , m_initialized(false)
{
    for (uint32_t i = 0; i < CSM_CASCADE_COUNT; ++i) {
        m_dsvHandles[i] = {};
        m_cascadeViewProj[i] = XMMatrixIdentity();
    }
}

CascadedShadowMapDX12::~CascadedShadowMapDX12()
{
    cleanup();
}

bool CascadedShadowMapDX12::init(ID3D12Device* device,
                                   ID3D12DescriptorHeap* dsvHeap,
                                   ID3D12DescriptorHeap* srvHeap,
                                   uint32_t dsvHeapStartIndex,
                                   uint32_t srvHeapIndex,
                                   uint32_t cascadeSize)
{
    if (m_initialized) {
        cleanup();
    }

    if (!device || !dsvHeap || !srvHeap) {
        std::cerr << "[CascadedShadowMapDX12] Invalid device or descriptor heaps!" << std::endl;
        return false;
    }

    m_device = device;
    m_cascadeSize = cascadeSize;
    m_dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // ========================================================================
    // Create Texture2DArray for all cascades
    // ========================================================================

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = cascadeSize;
    depthDesc.Height = cascadeSize;
    depthDesc.DepthOrArraySize = CSM_CASCADE_COUNT;  // Array of CASCADE_COUNT textures
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // Allow both DSV and SRV
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_shadowMapArray)
    );

    if (FAILED(hr)) {
        std::cerr << "[CascadedShadowMapDX12] Failed to create shadow map array! HRESULT: "
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    m_shadowMapArray->SetName(L"CascadedShadowMap_Array");
    m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    // ========================================================================
    // Create DSV for each cascade (array slice)
    // ========================================================================

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapStart = dsvHeap->GetCPUDescriptorHandleForHeapStart();

    for (uint32_t i = 0; i < CSM_CASCADE_COUNT; ++i) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = i;
        dsvDesc.Texture2DArray.ArraySize = 1;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        m_dsvHandles[i] = dsvHeapStart;
        m_dsvHandles[i].ptr += static_cast<SIZE_T>(dsvHeapStartIndex + i) * m_dsvDescriptorSize;

        device->CreateDepthStencilView(m_shadowMapArray.Get(), &dsvDesc, m_dsvHandles[i]);
    }

    // ========================================================================
    // Create SRV for entire array (for sampling in shaders)
    // ========================================================================

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = CSM_CASCADE_COUNT;
    srvDesc.Texture2DArray.PlaneSlice = 0;
    srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

    uint32_t srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_srvCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvCpuHandle.ptr += static_cast<SIZE_T>(srvHeapIndex) * srvDescriptorSize;

    m_srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    m_srvGpuHandle.ptr += static_cast<SIZE_T>(srvHeapIndex) * srvDescriptorSize;

    device->CreateShaderResourceView(m_shadowMapArray.Get(), &srvDesc, m_srvCpuHandle);

    // ========================================================================
    // Setup viewport and scissor
    // ========================================================================

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(cascadeSize);
    m_viewport.Height = static_cast<float>(cascadeSize);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(cascadeSize);
    m_scissorRect.bottom = static_cast<LONG>(cascadeSize);

    m_initialized = true;
    std::cout << "[CascadedShadowMapDX12] Initialized " << CSM_CASCADE_COUNT << " cascades at "
              << cascadeSize << "x" << cascadeSize << " each (D32_FLOAT)" << std::endl;

    return true;
}

void CascadedShadowMapDX12::cleanup()
{
    m_shadowMapArray.Reset();
    for (uint32_t i = 0; i < CSM_CASCADE_COUNT; ++i) {
        m_dsvHandles[i] = {};
    }
    m_srvCpuHandle = {};
    m_srvGpuHandle = {};
    m_initialized = false;
}

void CascadedShadowMapDX12::calculateFrustumCorners(const XMMATRIX& invViewProj,
                                                     float nearSplit, float farSplit,
                                                     XMFLOAT3 corners[8]) const
{
    // NDC corners for a frustum slice
    // Near plane corners (z = nearSplit mapped to NDC)
    // Far plane corners (z = farSplit mapped to NDC)

    // For DirectX, NDC z is [0, 1] (near = 0, far = 1)
    // We need to convert view-space depths to NDC depths
    // But since we're using invViewProj, we work in NDC directly

    // 8 corners of the frustum in NDC space, then transform to world
    const XMFLOAT3 ndcCorners[8] = {
        // Near plane (z = 0 in NDC for DirectX)
        {-1.0f, -1.0f, 0.0f}, // bottom-left
        { 1.0f, -1.0f, 0.0f}, // bottom-right
        { 1.0f,  1.0f, 0.0f}, // top-right
        {-1.0f,  1.0f, 0.0f}, // top-left
        // Far plane (z = 1 in NDC for DirectX)
        {-1.0f, -1.0f, 1.0f}, // bottom-left
        { 1.0f, -1.0f, 1.0f}, // bottom-right
        { 1.0f,  1.0f, 1.0f}, // top-right
        {-1.0f,  1.0f, 1.0f}  // top-left
    };

    // Transform from NDC to world space
    for (int i = 0; i < 8; ++i) {
        XMVECTOR corner = XMLoadFloat3(&ndcCorners[i]);
        corner = XMVector3TransformCoord(corner, invViewProj);
        XMStoreFloat3(&corners[i], corner);
    }

    // Now interpolate between near and far planes based on split distances
    // nearSplit and farSplit are normalized [0, 1] representing the depth range
    for (int i = 0; i < 4; ++i) {
        // Interpolate near corners
        XMVECTOR nearCorner = XMLoadFloat3(&corners[i]);
        XMVECTOR farCorner = XMLoadFloat3(&corners[i + 4]);
        XMVECTOR newNear = XMVectorLerp(nearCorner, farCorner, nearSplit);
        XMVECTOR newFar = XMVectorLerp(nearCorner, farCorner, farSplit);
        XMStoreFloat3(&corners[i], newNear);
        XMStoreFloat3(&corners[i + 4], newFar);
    }
}

void CascadedShadowMapDX12::calculateCascadeBounds(const XMFLOAT3 corners[8],
                                                    const XMFLOAT3& lightDir,
                                                    XMMATRIX& lightViewProj,
                                                    float& nearZ, float& farZ) const
{
    // Calculate frustum center
    XMVECTOR center = XMVectorZero();
    for (int i = 0; i < 8; ++i) {
        center = XMVectorAdd(center, XMLoadFloat3(&corners[i]));
    }
    center = XMVectorScale(center, 1.0f / 8.0f);

    // Calculate bounding sphere radius for stability
    float radius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMLoadFloat3(&corners[i]), center)));
        radius = std::max(radius, dist);
    }

    // Round up to reduce shadow swimming
    radius = std::ceil(radius * 16.0f) / 16.0f;

    // Create light view matrix
    XMVECTOR lightDirection = XMVector3Normalize(XMLoadFloat3(&lightDir));
    XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(lightDirection, radius * 2.0f));

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    float dotResult;
    XMStoreFloat(&dotResult, XMVector3Dot(lightDirection, up));
    if (fabsf(dotResult) > 0.99f) {
        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, center, up);

    // Calculate AABB in light space for tighter bounds
    XMFLOAT3 minBounds = { FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 maxBounds = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (int i = 0; i < 8; ++i) {
        XMVECTOR cornerWorld = XMLoadFloat3(&corners[i]);
        XMVECTOR cornerLight = XMVector3TransformCoord(cornerWorld, lightView);
        XMFLOAT3 cl;
        XMStoreFloat3(&cl, cornerLight);

        minBounds.x = std::min(minBounds.x, cl.x);
        minBounds.y = std::min(minBounds.y, cl.y);
        minBounds.z = std::min(minBounds.z, cl.z);
        maxBounds.x = std::max(maxBounds.x, cl.x);
        maxBounds.y = std::max(maxBounds.y, cl.y);
        maxBounds.z = std::max(maxBounds.z, cl.z);
    }

    // Add some padding to near/far planes
    nearZ = minBounds.z - radius * 0.5f;
    farZ = maxBounds.z + radius * 0.5f;

    // Create orthographic projection
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(
        minBounds.x, maxBounds.x,
        minBounds.y, maxBounds.y,
        nearZ, farZ
    );

    // Texel snapping to reduce shadow shimmering
    XMMATRIX shadowMatrix = XMMatrixMultiply(lightView, lightProj);
    XMVECTOR shadowOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    shadowOrigin = XMVector4Transform(shadowOrigin, shadowMatrix);
    shadowOrigin = XMVectorScale(shadowOrigin, static_cast<float>(m_cascadeSize) / 2.0f);

    XMFLOAT4 originSnapped;
    XMStoreFloat4(&originSnapped, shadowOrigin);
    originSnapped.x = std::round(originSnapped.x);
    originSnapped.y = std::round(originSnapped.y);

    XMFLOAT4 originOrig;
    XMStoreFloat4(&originOrig, shadowOrigin);
    float offsetX = (originSnapped.x - originOrig.x) * 2.0f / static_cast<float>(m_cascadeSize);
    float offsetY = (originSnapped.y - originOrig.y) * 2.0f / static_cast<float>(m_cascadeSize);

    lightProj.r[3] = XMVectorAdd(lightProj.r[3], XMVectorSet(offsetX, offsetY, 0.0f, 0.0f));

    lightViewProj = XMMatrixMultiply(lightView, lightProj);
}

void CascadedShadowMapDX12::updateCascades(const XMMATRIX& cameraView,
                                            const XMMATRIX& cameraProjection,
                                            const XMFLOAT3& lightDir,
                                            float nearPlane,
                                            float farPlane)
{
    if (!m_initialized) return;

    // Compute inverse view-projection for transforming NDC to world
    XMMATRIX viewProj = XMMatrixMultiply(cameraView, cameraProjection);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    // Calculate cascade split distances as normalized values [0, 1]
    float clipRange = farPlane - nearPlane;

    // Process each cascade
    float prevSplitDist = 0.0f;
    for (uint32_t i = 0; i < CSM_CASCADE_COUNT; ++i) {
        // Convert split distance to normalized [0, 1] range
        float splitDist = m_cascadeSplits[i] / farPlane;
        splitDist = std::min(splitDist, 1.0f);

        // Calculate frustum corners for this cascade's depth range
        XMFLOAT3 frustumCorners[8];
        calculateFrustumCorners(invViewProj, prevSplitDist, splitDist, frustumCorners);

        // Calculate tight light-space bounds
        float nearZ, farZ;
        calculateCascadeBounds(frustumCorners, lightDir, m_cascadeViewProj[i], nearZ, farZ);

        prevSplitDist = splitDist;
    }
}

void CascadedShadowMapDX12::beginCascade(uint32_t cascadeIndex, ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList || cascadeIndex >= CSM_CASCADE_COUNT) {
        return;
    }

    m_currentCascade = cascadeIndex;

    // Transition to depth write if needed (only on first cascade)
    if (cascadeIndex == 0 && m_currentState != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_shadowMapArray.Get();
        barrier.Transition.StateBefore = m_currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    // Clear this cascade's depth
    cmdList->ClearDepthStencilView(m_dsvHandles[cascadeIndex], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set viewport and scissor
    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissorRect);

    // Set depth-only render target for this cascade
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &m_dsvHandles[cascadeIndex]);
}

void CascadedShadowMapDX12::endCascade(ID3D12GraphicsCommandList* cmdList)
{
    // Nothing to do here - batched transition in endShadowPass
    (void)cmdList;
}

void CascadedShadowMapDX12::endShadowPass(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList) {
        return;
    }

    // Transition to shader resource for sampling
    if (m_currentState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_shadowMapArray.Get();
        barrier.Transition.StateBefore = m_currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        m_currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

const XMMATRIX& CascadedShadowMapDX12::getCascadeViewProj(uint32_t cascadeIndex) const
{
    static XMMATRIX identity = XMMatrixIdentity();
    if (cascadeIndex >= CSM_CASCADE_COUNT) {
        return identity;
    }
    return m_cascadeViewProj[cascadeIndex];
}

XMFLOAT4X4 CascadedShadowMapDX12::getCascadeViewProjFloat4x4(uint32_t cascadeIndex) const
{
    XMFLOAT4X4 result;
    if (cascadeIndex >= CSM_CASCADE_COUNT) {
        XMStoreFloat4x4(&result, XMMatrixIdentity());
    } else {
        XMStoreFloat4x4(&result, m_cascadeViewProj[cascadeIndex]);
    }
    return result;
}

CSMConstants CascadedShadowMapDX12::getCSMConstants() const
{
    CSMConstants constants = {};

    for (uint32_t i = 0; i < CSM_CASCADE_COUNT; ++i) {
        XMStoreFloat4x4(&constants.cascadeViewProj[i], m_cascadeViewProj[i]);
    }

    // Store split distances for shader cascade selection
    constants.cascadeSplits = XMFLOAT4(
        m_cascadeSplits[0],
        m_cascadeSplits[1],
        m_cascadeSplits[2],
        m_cascadeSplits[3]
    );

    // Atlas offsets/scales - not used when using Texture2DArray, but kept for compatibility
    for (uint32_t i = 0; i < CSM_CASCADE_COUNT; ++i) {
        constants.cascadeOffsets[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        constants.cascadeScales[i] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    return constants;
}

void CascadedShadowMapDX12::setCascadeSplits(const std::array<float, CSM_CASCADE_COUNT>& splits)
{
    m_cascadeSplits = splits;
}

D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapDX12::getDSVHandle(uint32_t cascadeIndex) const
{
    if (cascadeIndex >= CSM_CASCADE_COUNT) {
        return {};
    }
    return m_dsvHandles[cascadeIndex];
}
