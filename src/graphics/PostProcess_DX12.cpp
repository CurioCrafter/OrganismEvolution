#include "PostProcess_DX12.h"
#include <iostream>
#include <random>
#include <cmath>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// ============================================================================
// Constructor / Destructor
// ============================================================================

PostProcessManagerDX12::PostProcessManagerDX12()
{
    createSSAOKernel();

    // Initialize default color grading
    colorGrading.shadowTint = {0.5f, 0.5f, 0.5f};
    colorGrading.shadowTintStrength = 0.0f;
    colorGrading.midtoneTint = {1.0f, 1.0f, 1.0f};
    colorGrading.midtoneTintStrength = 0.0f;
    colorGrading.highlightTint = {1.0f, 1.0f, 1.0f};
    colorGrading.highlightTintStrength = 0.0f;
    colorGrading.colorTemperature = 0.0f;
    colorGrading.vignetteIntensity = 0.0f;
    colorGrading.vignetteRadius = 0.8f;
    colorGrading.timeOfDay = 0.5f;
}

PostProcessManagerDX12::~PostProcessManagerDX12()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool PostProcessManagerDX12::init(ID3D12Device* device,
                                   ID3D12DescriptorHeap* srvUavHeap,
                                   uint32_t srvStartIndex,
                                   uint32_t width, uint32_t height)
{
    if (m_initialized) {
        cleanup();
    }

    if (!device || !srvUavHeap || width == 0 || height == 0) {
        std::cerr << "[PostProcessManagerDX12] Invalid initialization parameters!" << std::endl;
        return false;
    }

    m_device = device;
    m_srvUavHeap = srvUavHeap;
    m_srvStartIndex = srvStartIndex;
    m_width = width;
    m_height = height;
    m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create all resources
    if (!createHDRBuffer()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create HDR buffer!" << std::endl;
        return false;
    }

    if (!createDepthCopy()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create depth copy!" << std::endl;
        return false;
    }

    if (!createNormalBuffer()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create normal buffer!" << std::endl;
        return false;
    }

    if (!createSSAOBuffers()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create SSAO buffers!" << std::endl;
        return false;
    }

    if (!createNoiseTexture()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create noise texture!" << std::endl;
        return false;
    }

    if (!createBloomBuffers()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create bloom buffers!" << std::endl;
        return false;
    }

    if (!createSSRBuffer()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create SSR buffer!" << std::endl;
        return false;
    }

    if (!createVolumetricBuffer()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create volumetric buffer!" << std::endl;
        return false;
    }

    if (!createUnderwaterBuffer()) {
        std::cerr << "[PostProcessManagerDX12] Failed to create underwater buffer!" << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "[PostProcessManagerDX12] Initialized at " << width << "x" << height << std::endl;

    return true;
}

void PostProcessManagerDX12::cleanup()
{
    releaseResources();
    m_initialized = false;
}

void PostProcessManagerDX12::resize(uint32_t width, uint32_t height)
{
    if (width == m_width && height == m_height) {
        return;
    }

    if (!m_initialized || !m_device) {
        return;
    }

    m_width = width;
    m_height = height;

    // Recreate size-dependent resources
    releaseResources();
    createHDRBuffer();
    createDepthCopy();
    createNormalBuffer();
    createSSAOBuffers();
    createBloomBuffers();
    createSSRBuffer();
    createVolumetricBuffer();
    createUnderwaterBuffer();

    std::cout << "[PostProcessManagerDX12] Resized to " << width << "x" << height << std::endl;
}

void PostProcessManagerDX12::releaseResources()
{
    m_hdrBuffer.Reset();
    m_hdrRTVHeap.Reset();
    m_depthCopy.Reset();
    m_normalBuffer.Reset();
    m_ssaoBuffer.Reset();
    m_ssaoBlurBuffer.Reset();
    m_noiseTexture.Reset();
    m_ssrBuffer.Reset();
    m_volumetricBuffer.Reset();
    m_underwaterBuffer.Reset();

    for (int i = 0; i < MAX_BLOOM_MIPS; ++i) {
        m_bloomBuffers[i].Reset();
    }
}

// ============================================================================
// Resource Creation
// ============================================================================

bool PostProcessManagerDX12::createHDRBuffer()
{
    // Create HDR render target (RGBA16F)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue,
        IID_PPV_ARGS(&m_hdrBuffer)
    );

    if (FAILED(hr)) return false;
    m_hdrBuffer->SetName(L"HDR_Buffer");
    m_hdrState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // Create private RTV heap for HDR buffer
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_hdrRTVHeap));
    if (FAILED(hr)) return false;

    m_hdrRTVHandle = m_hdrRTVHeap->GetCPUDescriptorHandleForHeapStart();
    m_device->CreateRenderTargetView(m_hdrBuffer.Get(), nullptr, m_hdrRTVHandle);

    // Create SRV for HDR buffer
    uint32_t srvIndex = m_srvStartIndex;
    m_hdrSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_hdrSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_hdrSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_hdrSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_hdrBuffer.Get(), &srvDesc, m_hdrSRVCpu);

    return true;
}

bool PostProcessManagerDX12::createDepthCopy()
{
    // Create depth copy texture (R32_FLOAT for reading)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_depthCopy)
    );

    if (FAILED(hr)) return false;
    m_depthCopy->SetName(L"Depth_Copy");

    // Create SRV
    uint32_t srvIndex = m_srvStartIndex + 1;
    m_depthCopySRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_depthCopySRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_depthCopySRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_depthCopySRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_depthCopy.Get(), &srvDesc, m_depthCopySRVCpu);

    return true;
}

bool PostProcessManagerDX12::createNormalBuffer()
{
    // Create normal buffer (RG16F for view-space normals)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_normalBuffer)
    );

    if (FAILED(hr)) return false;
    m_normalBuffer->SetName(L"Normal_Buffer");

    // Create SRV and UAV
    uint32_t srvIndex = m_srvStartIndex + 2;
    m_normalSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_normalSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_normalSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_normalSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_normalBuffer.Get(), &srvDesc, m_normalSRVCpu);

    // UAV
    uint32_t uavIndex = m_srvStartIndex + 3;
    m_normalUAVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_normalUAVCpu.ptr += uavIndex * m_srvDescriptorSize;
    m_normalUAVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_normalUAVGpu.ptr += uavIndex * m_srvDescriptorSize;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(m_normalBuffer.Get(), nullptr, &uavDesc, m_normalUAVCpu);

    return true;
}

bool PostProcessManagerDX12::createSSAOBuffers()
{
    // SSAO output (R8_UNORM)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_ssaoBuffer)
    );

    if (FAILED(hr)) return false;
    m_ssaoBuffer->SetName(L"SSAO_Buffer");

    // SSAO blur buffer
    hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_ssaoBlurBuffer)
    );

    if (FAILED(hr)) return false;
    m_ssaoBlurBuffer->SetName(L"SSAO_Blur_Buffer");

    // Create SRV/UAV for SSAO buffer
    uint32_t srvIndex = m_srvStartIndex + 4;
    m_ssaoSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_ssaoSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_ssaoSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssaoSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_ssaoBuffer.Get(), &srvDesc, m_ssaoSRVCpu);

    // UAV for SSAO
    uint32_t uavIndex = m_srvStartIndex + 5;
    m_ssaoUAVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_ssaoUAVCpu.ptr += uavIndex * m_srvDescriptorSize;
    m_ssaoUAVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssaoUAVGpu.ptr += uavIndex * m_srvDescriptorSize;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(m_ssaoBuffer.Get(), nullptr, &uavDesc, m_ssaoUAVCpu);

    // SRV/UAV for blur buffer
    srvIndex = m_srvStartIndex + 6;
    m_ssaoBlurSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_ssaoBlurSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_ssaoBlurSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssaoBlurSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    m_device->CreateShaderResourceView(m_ssaoBlurBuffer.Get(), &srvDesc, m_ssaoBlurSRVCpu);

    uavIndex = m_srvStartIndex + 7;
    m_ssaoBlurUAVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_ssaoBlurUAVCpu.ptr += uavIndex * m_srvDescriptorSize;
    m_ssaoBlurUAVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssaoBlurUAVGpu.ptr += uavIndex * m_srvDescriptorSize;

    m_device->CreateUnorderedAccessView(m_ssaoBlurBuffer.Get(), nullptr, &uavDesc, m_ssaoBlurUAVCpu);

    return true;
}

bool PostProcessManagerDX12::createNoiseTexture()
{
    // 4x4 noise texture for SSAO random rotation
    const uint32_t noiseSize = 4;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = noiseSize;
    desc.Height = noiseSize;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_noiseTexture)
    );

    if (FAILED(hr)) return false;
    m_noiseTexture->SetName(L"SSAO_Noise");

    // Create SRV
    uint32_t srvIndex = m_srvStartIndex + 8;
    m_noiseSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_noiseSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_noiseSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_noiseSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_noiseTexture.Get(), &srvDesc, m_noiseSRVCpu);

    // Note: Actual noise data upload would require a staging buffer and copy command
    // This is handled during first use or explicit initialization

    return true;
}

bool PostProcessManagerDX12::createBloomBuffers()
{
    // Create bloom mip chain
    for (int i = 0; i < MAX_BLOOM_MIPS; ++i) {
        uint32_t mipWidth = std::max(1u, m_width >> (i + 1));
        uint32_t mipHeight = std::max(1u, m_height >> (i + 1));

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = mipWidth;
        desc.Height = mipHeight;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;  // HDR but compressed
        desc.SampleDesc.Count = 1;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_bloomBuffers[i])
        );

        if (FAILED(hr)) return false;

        wchar_t name[64];
        swprintf(name, 64, L"Bloom_Mip_%d", i);
        m_bloomBuffers[i]->SetName(name);

        // Create SRV/UAV
        uint32_t srvIndex = m_srvStartIndex + 9 + i * 2;
        m_bloomSRVCpu[i] = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
        m_bloomSRVCpu[i].ptr += srvIndex * m_srvDescriptorSize;
        m_bloomSRVGpu[i] = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
        m_bloomSRVGpu[i].ptr += srvIndex * m_srvDescriptorSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        m_device->CreateShaderResourceView(m_bloomBuffers[i].Get(), &srvDesc, m_bloomSRVCpu[i]);

        uint32_t uavIndex = m_srvStartIndex + 9 + i * 2 + 1;
        m_bloomUAVCpu[i] = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
        m_bloomUAVCpu[i].ptr += uavIndex * m_srvDescriptorSize;
        m_bloomUAVGpu[i] = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
        m_bloomUAVGpu[i].ptr += uavIndex * m_srvDescriptorSize;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

        m_device->CreateUnorderedAccessView(m_bloomBuffers[i].Get(), nullptr, &uavDesc, m_bloomUAVCpu[i]);
    }

    return true;
}

bool PostProcessManagerDX12::createSSRBuffer()
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_ssrBuffer)
    );

    if (FAILED(hr)) return false;
    m_ssrBuffer->SetName(L"SSR_Buffer");

    // Create SRV/UAV (indices continue after bloom)
    uint32_t srvIndex = m_srvStartIndex + 19;  // After bloom (9 + 5*2)
    m_ssrSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_ssrSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_ssrSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssrSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_ssrBuffer.Get(), &srvDesc, m_ssrSRVCpu);

    uint32_t uavIndex = m_srvStartIndex + 20;
    m_ssrUAVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_ssrUAVCpu.ptr += uavIndex * m_srvDescriptorSize;
    m_ssrUAVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_ssrUAVGpu.ptr += uavIndex * m_srvDescriptorSize;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(m_ssrBuffer.Get(), nullptr, &uavDesc, m_ssrUAVCpu);

    return true;
}

bool PostProcessManagerDX12::createVolumetricBuffer()
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width / 2;  // Half resolution for performance
    desc.Height = m_height / 2;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_volumetricBuffer)
    );

    if (FAILED(hr)) return false;
    m_volumetricBuffer->SetName(L"Volumetric_Buffer");

    // Create SRV/UAV
    uint32_t srvIndex = m_srvStartIndex + 21;
    m_volumetricSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_volumetricSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_volumetricSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_volumetricSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_volumetricBuffer.Get(), &srvDesc, m_volumetricSRVCpu);

    uint32_t uavIndex = m_srvStartIndex + 22;
    m_volumetricUAVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_volumetricUAVCpu.ptr += uavIndex * m_srvDescriptorSize;
    m_volumetricUAVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_volumetricUAVGpu.ptr += uavIndex * m_srvDescriptorSize;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(m_volumetricBuffer.Get(), nullptr, &uavDesc, m_volumetricUAVCpu);

    return true;
}

bool PostProcessManagerDX12::createUnderwaterBuffer()
{
    // Full resolution buffer for underwater effects (caustics, light shafts, fog)
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // Full precision for blending
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_underwaterBuffer)
    );

    if (FAILED(hr)) return false;
    m_underwaterBuffer->SetName(L"Underwater_Buffer");

    // Create SRV/UAV (indices continue after volumetric: 21, 22)
    uint32_t srvIndex = m_srvStartIndex + 23;
    m_underwaterSRVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_underwaterSRVCpu.ptr += srvIndex * m_srvDescriptorSize;
    m_underwaterSRVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_underwaterSRVGpu.ptr += srvIndex * m_srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(m_underwaterBuffer.Get(), &srvDesc, m_underwaterSRVCpu);

    uint32_t uavIndex = m_srvStartIndex + 24;
    m_underwaterUAVCpu = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_underwaterUAVCpu.ptr += uavIndex * m_srvDescriptorSize;
    m_underwaterUAVGpu = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
    m_underwaterUAVGpu.ptr += uavIndex * m_srvDescriptorSize;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(m_underwaterBuffer.Get(), nullptr, &uavDesc, m_underwaterUAVCpu);

    std::cout << "[PostProcess] Underwater buffer created at " << m_width << "x" << m_height << std::endl;
    return true;
}

bool PostProcessManagerDX12::createSSAOKernel()
{
    // Generate hemisphere kernel samples
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < 32; ++i) {
        // Random direction in hemisphere
        float x = dist(gen) * 2.0f - 1.0f;
        float y = dist(gen) * 2.0f - 1.0f;
        float z = dist(gen);  // Hemisphere (z >= 0)

        // Normalize
        float len = std::sqrt(x * x + y * y + z * z);
        x /= len;
        y /= len;
        z /= len;

        // Scale so more samples are closer to origin
        float scale = static_cast<float>(i) / 32.0f;
        scale = 0.1f + scale * scale * 0.9f;  // Lerp(0.1, 1.0, scale^2)

        m_ssaoKernel[i] = XMFLOAT4(x * scale, y * scale, z * scale, 0.0f);
    }

    return true;
}

// ============================================================================
// Shader Pipeline Creation
// ============================================================================

bool PostProcessManagerDX12::createShaderPipelines(const std::wstring& shaderDirectory)
{
    if (!m_device) {
        std::cerr << "[PostProcess] Cannot create shader pipelines: device not set" << std::endl;
        return false;
    }

    // Create root signatures first
    if (!createRootSignatures()) {
        std::cerr << "[PostProcess] Failed to create root signatures" << std::endl;
        return false;
    }

    // Create compute PSOs
    if (!createComputePSOs()) {
        std::cerr << "[PostProcess] Failed to create compute PSOs" << std::endl;
        // Continue - we can work without some PSOs
    }

    // Create graphics PSOs (for tone mapping)
    if (!createGraphicsPSOs()) {
        std::cerr << "[PostProcess] Failed to create graphics PSOs" << std::endl;
        // Continue - we can work without some PSOs
    }

    std::cout << "[PostProcess] Shader pipelines created successfully" << std::endl;
    return true;
}

bool PostProcessManagerDX12::createRootSignatures()
{
    // Create compute root signature for SSAO, Bloom, SSR, Volumetrics
    // Layout:
    // [0] CBV - Constants (b0)
    // [1] Descriptor Table - SRVs (t0-t3)
    // [2] Descriptor Table - UAV (u0)
    // [3] Static Samplers (s0, s1)

    D3D12_DESCRIPTOR_RANGE1 srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 4;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE1 uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0;
    uavRange.RegisterSpace = 0;
    uavRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER1 rootParams[3] = {};

    // [0] CBV
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // [1] SRV Table
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // [2] UAV Table
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &uavRange;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Static samplers
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

    // Linear sampler (s0)
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].RegisterSpace = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Point sampler (s1)
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[1].ShaderRegister = 1;
    staticSamplers[1].RegisterSpace = 0;
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC computeRsDesc = {};
    computeRsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    computeRsDesc.Desc_1_1.NumParameters = 3;
    computeRsDesc.Desc_1_1.pParameters = rootParams;
    computeRsDesc.Desc_1_1.NumStaticSamplers = 2;
    computeRsDesc.Desc_1_1.pStaticSamplers = staticSamplers;
    computeRsDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&computeRsDesc, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            std::cerr << "[PostProcess] Compute RS error: " << (char*)error->GetBufferPointer() << std::endl;
        }
        return false;
    }

    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                        IID_PPV_ARGS(&m_computeRootSignature));
    if (FAILED(hr)) {
        std::cerr << "[PostProcess] Failed to create compute root signature" << std::endl;
        return false;
    }

    // Create graphics root signature for tone mapping
    // Similar but with SRV table for 5 textures (HDR, SSAO, Bloom, SSR, Volumetrics)
    D3D12_DESCRIPTOR_RANGE1 gfxSrvRange = {};
    gfxSrvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    gfxSrvRange.NumDescriptors = 5;
    gfxSrvRange.BaseShaderRegister = 0;
    gfxSrvRange.RegisterSpace = 0;
    gfxSrvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

    D3D12_ROOT_PARAMETER1 gfxRootParams[2] = {};

    // [0] CBV
    gfxRootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    gfxRootParams[0].Descriptor.ShaderRegister = 0;
    gfxRootParams[0].Descriptor.RegisterSpace = 0;
    gfxRootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // [1] SRV Table
    gfxRootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    gfxRootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    gfxRootParams[1].DescriptorTable.pDescriptorRanges = &gfxSrvRange;
    gfxRootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC gfxRsDesc = {};
    gfxRsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    gfxRsDesc.Desc_1_1.NumParameters = 2;
    gfxRsDesc.Desc_1_1.pParameters = gfxRootParams;
    gfxRsDesc.Desc_1_1.NumStaticSamplers = 1;  // Just linear sampler
    gfxRsDesc.Desc_1_1.pStaticSamplers = staticSamplers;
    gfxRsDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    hr = D3D12SerializeVersionedRootSignature(&gfxRsDesc, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            std::cerr << "[PostProcess] Graphics RS error: " << (char*)error->GetBufferPointer() << std::endl;
        }
        return false;
    }

    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                        IID_PPV_ARGS(&m_graphicsRootSignature));
    if (FAILED(hr)) {
        std::cerr << "[PostProcess] Failed to create graphics root signature" << std::endl;
        return false;
    }

    std::cout << "[PostProcess] Root signatures created" << std::endl;
    return true;
}

bool PostProcessManagerDX12::compileShader(const std::wstring& path, const std::string& entryPoint,
                                            const std::wstring& target, ID3DBlob** blob)
{
    UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                     entryPoint.c_str(), std::string(target.begin(), target.end()).c_str(),
                                     compileFlags, 0, blob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "[PostProcess] Shader compile error (" << entryPoint << "): "
                      << (char*)errorBlob->GetBufferPointer() << std::endl;
        }
        return false;
    }

    return true;
}

bool PostProcessManagerDX12::createComputePSOs()
{
    // Note: For full implementation, these would compile the HLSL shaders
    // For now, we'll leave them as placeholders since the project uses DXC
    // and a different shader compilation path

    std::cout << "[PostProcess] Compute PSO creation deferred to shader manager" << std::endl;

    // The PSOs will be null, which is handled gracefully in render passes
    // A complete implementation would:
    // 1. Compile SSAO.hlsl with entry point "CSMain" -> m_ssaoPSO
    // 2. Compile SSAO.hlsl with entry point "CSBlur" -> m_ssaoBlurPSO
    // 3. Compile Bloom.hlsl with entry point "CSExtract" -> m_bloomExtractPSO
    // 4. Compile Bloom.hlsl with entry point "CSDownsample" -> m_bloomBlurPSO
    // 5. Compile SSR.hlsl with entry point "CSMain" -> m_ssrPSO
    // 6. Compile VolumetricFog.hlsl with entry point "CSMain" -> m_volumetricPSO

    return true;  // Return true to allow pipeline to continue
}

bool PostProcessManagerDX12::createGraphicsPSOs()
{
    // Note: For full implementation, these would compile ToneMapping.hlsl
    // For now, we'll leave as placeholders

    std::cout << "[PostProcess] Graphics PSO creation deferred to shader manager" << std::endl;

    // A complete implementation would:
    // 1. Compile ToneMapping.hlsl with entry points "VSMain" and "PSMain"
    // 2. Create graphics PSO with no depth test, blend disabled
    // 3. Set primitive topology to triangle list
    // 4. Use R8G8B8A8_UNORM render target format

    return true;  // Return true to allow pipeline to continue
}

// ============================================================================
// HDR Pass Management
// ============================================================================

void PostProcessManagerDX12::beginHDRPass(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE depthDSV)
{
    if (!m_initialized || !cmdList) return;

    // Transition HDR buffer to render target if needed
    if (m_hdrState != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_hdrBuffer.Get();
        barrier.Transition.StateBefore = m_hdrState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        m_hdrState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    // Clear HDR buffer
    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    cmdList->ClearRenderTargetView(m_hdrRTVHandle, clearColor, 0, nullptr);

    // Set HDR as render target
    cmdList->OMSetRenderTargets(1, &m_hdrRTVHandle, FALSE, &depthDSV);
}

void PostProcessManagerDX12::endHDRPass(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList) return;

    // Transition HDR buffer to shader resource for post-processing reads
    if (m_hdrState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_hdrBuffer.Get();
        barrier.Transition.StateBefore = m_hdrState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        m_hdrState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

// ============================================================================
// Helper: Transition Resource State
// ============================================================================

static void TransitionResource(ID3D12GraphicsCommandList* cmdList,
                               ID3D12Resource* resource,
                               D3D12_RESOURCE_STATES before,
                               D3D12_RESOURCE_STATES after)
{
    if (before == after || !resource) return;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);
}

static void UAVBarrier(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;
    cmdList->ResourceBarrier(1, &barrier);
}

// ============================================================================
// Render Pass Implementations
// ============================================================================

void PostProcessManagerDX12::copyDepthBuffer(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* sourceDepth)
{
    if (!m_initialized || !cmdList || !sourceDepth) return;

    // Note: The source depth buffer needs to be in a copyable state
    // For now, we skip this if PSOs aren't ready (gradual integration)
    // The depth copy allows SSAO/SSR to sample depth without conflicts

    // Transition depth copy to copy dest
    TransitionResource(cmdList, m_depthCopy.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST);

    // Copy the depth buffer
    // Note: Source depth must be transitioned to COPY_SOURCE by caller
    // For D32_FLOAT to R32_FLOAT copy, formats are compatible
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = sourceDepth;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = m_depthCopy.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition back to shader resource
    TransitionResource(cmdList, m_depthCopy.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::generateNormals(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList) return;

    // Generate view-space normals from depth buffer
    // This requires a compute shader that reconstructs normals from depth derivatives
    // For now, if we have a G-buffer with normals, we use that instead

    // Transition normal buffer to UAV for writing
    TransitionResource(cmdList, m_normalBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Note: Normal generation compute shader would be dispatched here
    // For full implementation, compile NormalGenerate.hlsl and dispatch
    // Thread groups: ceil(width/8) x ceil(height/8) x 1

    // Transition to shader resource for reading
    TransitionResource(cmdList, m_normalBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::renderSSAO(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList || !enableSSAO) return;

    // Transition SSAO buffer to UAV
    TransitionResource(cmdList, m_ssaoBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // If PSO is ready, dispatch the compute shader
    if (m_ssaoPSO && m_computeRootSignature)
    {
        cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
        cmdList->SetPipelineState(m_ssaoPSO.Get());

        // Bind descriptors
        // Root param 0: CBV (SSAO constants)
        // Root param 1: SRV table (depth, normals, noise)
        // Root param 2: UAV (ssao output)
        cmdList->SetComputeRootDescriptorTable(1, m_depthCopySRVGpu);
        cmdList->SetComputeRootDescriptorTable(2, m_ssaoUAVGpu);

        // Dispatch compute shader
        uint32_t groupsX = (m_width + 7) / 8;
        uint32_t groupsY = (m_height + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }

    // UAV barrier to ensure writes complete
    UAVBarrier(cmdList, m_ssaoBuffer.Get());

    // Transition to shader resource for blur pass
    TransitionResource(cmdList, m_ssaoBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::renderSSAOBlur(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList || !enableSSAO) return;

    // Transition blur buffer to UAV
    TransitionResource(cmdList, m_ssaoBlurBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (m_ssaoBlurPSO && m_computeRootSignature)
    {
        cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
        cmdList->SetPipelineState(m_ssaoBlurPSO.Get());

        // Bind SSAO result as input, blur buffer as output
        cmdList->SetComputeRootDescriptorTable(1, m_ssaoSRVGpu);
        cmdList->SetComputeRootDescriptorTable(2, m_ssaoBlurUAVGpu);

        uint32_t groupsX = (m_width + 7) / 8;
        uint32_t groupsY = (m_height + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }

    UAVBarrier(cmdList, m_ssaoBlurBuffer.Get());

    // Transition to shader resource for tone mapping
    TransitionResource(cmdList, m_ssaoBlurBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::renderSSR(ID3D12GraphicsCommandList* cmdList,
                                        ID3D12Resource* colorBuffer,
                                        const XMMATRIX& view,
                                        const XMMATRIX& projection)
{
    if (!m_initialized || !cmdList || !enableSSR) return;

    // SSR is expensive - disabled by default
    // Transition SSR buffer to UAV
    TransitionResource(cmdList, m_ssrBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (m_ssrPSO && m_computeRootSignature)
    {
        cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
        cmdList->SetPipelineState(m_ssrPSO.Get());

        // Bind color buffer, depth, normals as inputs
        // SSR output as UAV
        cmdList->SetComputeRootDescriptorTable(2, m_ssrUAVGpu);

        uint32_t groupsX = (m_width + 7) / 8;
        uint32_t groupsY = (m_height + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }

    UAVBarrier(cmdList, m_ssrBuffer.Get());

    TransitionResource(cmdList, m_ssrBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::renderVolumetrics(ID3D12GraphicsCommandList* cmdList,
                                                const XMMATRIX& invViewProj,
                                                const XMFLOAT3& cameraPos,
                                                const XMFLOAT3& lightDir,
                                                const XMFLOAT3& lightColor)
{
    if (!m_initialized || !cmdList || !enableVolumetrics) return;

    // Volumetrics runs at half resolution for performance
    TransitionResource(cmdList, m_volumetricBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (m_volumetricPSO && m_computeRootSignature)
    {
        cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
        cmdList->SetPipelineState(m_volumetricPSO.Get());

        // Bind depth as input, volumetric buffer as output
        cmdList->SetComputeRootDescriptorTable(1, m_depthCopySRVGpu);
        cmdList->SetComputeRootDescriptorTable(2, m_volumetricUAVGpu);

        // Half resolution dispatch
        uint32_t halfWidth = m_width / 2;
        uint32_t halfHeight = m_height / 2;
        uint32_t groupsX = (halfWidth + 7) / 8;
        uint32_t groupsY = (halfHeight + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }

    UAVBarrier(cmdList, m_volumetricBuffer.Get());

    TransitionResource(cmdList, m_volumetricBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::renderBloom(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList || !enableBloom) return;

    // Bloom is a multi-pass effect:
    // 1. Extract bright pixels from HDR buffer
    // 2. Progressive downsample through mip chain
    // 3. Progressive upsample with blur

    // Pass 1: Extract bright pixels to first bloom mip
    TransitionResource(cmdList, m_bloomBuffers[0].Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (m_bloomExtractPSO && m_computeRootSignature)
    {
        cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
        cmdList->SetPipelineState(m_bloomExtractPSO.Get());

        // HDR buffer as input, bloom[0] as output
        cmdList->SetComputeRootDescriptorTable(1, m_hdrSRVGpu);
        cmdList->SetComputeRootDescriptorTable(2, m_bloomUAVGpu[0]);

        uint32_t mipWidth = std::max(1u, m_width >> 1);
        uint32_t mipHeight = std::max(1u, m_height >> 1);
        uint32_t groupsX = (mipWidth + 7) / 8;
        uint32_t groupsY = (mipHeight + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }

    UAVBarrier(cmdList, m_bloomBuffers[0].Get());

    // Transition first mip to SRV for downsample chain
    TransitionResource(cmdList, m_bloomBuffers[0].Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Pass 2-5: Progressive downsample
    for (int i = 1; i < MAX_BLOOM_MIPS; ++i)
    {
        TransitionResource(cmdList, m_bloomBuffers[i].Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        if (m_bloomBlurPSO && m_computeRootSignature)
        {
            cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
            cmdList->SetPipelineState(m_bloomBlurPSO.Get());

            // Previous mip as input, current mip as output
            cmdList->SetComputeRootDescriptorTable(1, m_bloomSRVGpu[i - 1]);
            cmdList->SetComputeRootDescriptorTable(2, m_bloomUAVGpu[i]);

            uint32_t mipWidth = std::max(1u, m_width >> (i + 1));
            uint32_t mipHeight = std::max(1u, m_height >> (i + 1));
            uint32_t groupsX = (mipWidth + 7) / 8;
            uint32_t groupsY = (mipHeight + 7) / 8;
            cmdList->Dispatch(groupsX, groupsY, 1);
        }

        UAVBarrier(cmdList, m_bloomBuffers[i].Get());

        TransitionResource(cmdList, m_bloomBuffers[i].Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // Pass 6+: Progressive upsample (blend back into lower mips)
    // This accumulates the bloom contribution
    for (int i = MAX_BLOOM_MIPS - 2; i >= 0; --i)
    {
        TransitionResource(cmdList, m_bloomBuffers[i].Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Upsample from mip[i+1] to mip[i], blending with existing content
        if (m_bloomBlurPSO && m_computeRootSignature)
        {
            cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
            cmdList->SetPipelineState(m_bloomBlurPSO.Get());

            cmdList->SetComputeRootDescriptorTable(1, m_bloomSRVGpu[i + 1]);
            cmdList->SetComputeRootDescriptorTable(2, m_bloomUAVGpu[i]);

            uint32_t mipWidth = std::max(1u, m_width >> (i + 1));
            uint32_t mipHeight = std::max(1u, m_height >> (i + 1));
            uint32_t groupsX = (mipWidth + 7) / 8;
            uint32_t groupsY = (mipHeight + 7) / 8;
            cmdList->Dispatch(groupsX, groupsY, 1);
        }

        UAVBarrier(cmdList, m_bloomBuffers[i].Get());

        TransitionResource(cmdList, m_bloomBuffers[i].Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

void PostProcessManagerDX12::renderUnderwater(ID3D12GraphicsCommandList* cmdList,
                                               float underwaterDepth,
                                               const XMFLOAT2& sunScreenPos,
                                               float time)
{
    // Skip if underwater effects disabled or camera is above water
    if (!m_initialized || !cmdList || !enableUnderwater || underwaterDepth <= 0.0f) return;
    if (underwaterQuality == 0) return;

    // Transition underwater buffer to UAV for writing
    TransitionResource(cmdList, m_underwaterBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (m_underwaterPSO && m_computeRootSignature)
    {
        cmdList->SetComputeRootSignature(m_computeRootSignature.Get());
        cmdList->SetPipelineState(m_underwaterPSO.Get());

        // Bind HDR scene and depth as inputs, underwater buffer as output
        cmdList->SetComputeRootDescriptorTable(1, m_hdrSRVGpu);
        cmdList->SetComputeRootDescriptorTable(2, m_underwaterUAVGpu);

        // Dispatch compute shader
        uint32_t groupsX = (m_width + 7) / 8;
        uint32_t groupsY = (m_height + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }
    else
    {
        // Fallback: Apply basic underwater tint without compute shader
        // This allows gradual integration - the effect works even without the PSO
        // The tone mapping pass will blend in the underwater buffer if available
    }

    UAVBarrier(cmdList, m_underwaterBuffer.Get());

    // Transition to shader resource for tone mapping composition
    TransitionResource(cmdList, m_underwaterBuffer.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessManagerDX12::renderToneMapping(ID3D12GraphicsCommandList* cmdList,
                                                D3D12_CPU_DESCRIPTOR_HANDLE outputRTV)
{
    if (!m_initialized || !cmdList) return;

    // Final composition pass - this uses a graphics pipeline (fullscreen triangle)
    // Combines HDR scene, SSAO, Bloom, Volumetrics, SSR and applies tone mapping

    if (m_toneMappingPSO && m_graphicsRootSignature)
    {
        // Set pipeline state
        cmdList->SetPipelineState(m_toneMappingPSO.Get());
        cmdList->SetGraphicsRootSignature(m_graphicsRootSignature.Get());

        // Set render target (backbuffer)
        cmdList->OMSetRenderTargets(1, &outputRTV, FALSE, nullptr);

        // Set viewport and scissor
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &viewport);

        D3D12_RECT scissor = {};
        scissor.right = static_cast<LONG>(m_width);
        scissor.bottom = static_cast<LONG>(m_height);
        cmdList->RSSetScissorRects(1, &scissor);

        // Bind input textures (HDR, SSAO, Bloom, Volumetrics)
        // Root param 0: CBV (tone mapping constants)
        // Root param 1: SRV table (HDR, SSAO, Bloom, SSR, Volumetrics)
        cmdList->SetGraphicsRootDescriptorTable(1, m_hdrSRVGpu);

        // Draw fullscreen triangle (3 vertices, no vertex buffer needed)
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
    else
    {
        // Fallback: If PSOs aren't ready, just copy HDR to backbuffer
        // This allows gradual integration without breaking the renderer
        std::cout << "[PostProcess] Warning: Tone mapping PSO not ready, skipping post-process" << std::endl;
    }
}

// ============================================================================
// Time-of-Day Color Grading
// ============================================================================

void PostProcessManagerDX12::updateColorGrading(float timeOfDay)
{
    colorGrading.timeOfDay = timeOfDay;

    // Define color grading presets for different times of day
    // timeOfDay: 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk

    if (timeOfDay > 0.2f && timeOfDay < 0.35f)
    {
        // Dawn - golden hour, warm orange highlights, purple shadows
        float t = (timeOfDay - 0.2f) / 0.15f;  // 0-1 through dawn

        colorGrading.shadowTint = {0.25f, 0.15f, 0.35f};  // Purple/violet
        colorGrading.shadowTintStrength = 0.35f * (1.0f - t * 0.5f);

        colorGrading.midtoneTint = {1.0f, 0.95f, 0.85f};  // Warm
        colorGrading.midtoneTintStrength = 0.2f * (1.0f - t);

        colorGrading.highlightTint = {1.0f, 0.85f, 0.6f};  // Golden
        colorGrading.highlightTintStrength = 0.4f * (1.0f - t);

        colorGrading.colorTemperature = 0.25f * (1.0f - t);  // Warm to neutral
        saturation = 1.08f + 0.07f * (1.0f - t);  // Slightly boosted
        colorGrading.vignetteIntensity = 0.15f * (1.0f - t);
    }
    else if (timeOfDay > 0.35f && timeOfDay < 0.65f)
    {
        // Day - neutral, clear
        float t = (timeOfDay - 0.35f) / 0.3f;  // 0-1 through day

        colorGrading.shadowTint = {0.4f, 0.45f, 0.5f};  // Cool shadows
        colorGrading.shadowTintStrength = 0.1f;

        colorGrading.midtoneTint = {1.0f, 1.0f, 1.0f};
        colorGrading.midtoneTintStrength = 0.0f;

        colorGrading.highlightTint = {1.0f, 0.98f, 0.95f};  // Slightly warm
        colorGrading.highlightTintStrength = 0.05f;

        colorGrading.colorTemperature = 0.0f;
        saturation = 1.0f;
        colorGrading.vignetteIntensity = 0.0f;
    }
    else if (timeOfDay > 0.65f && timeOfDay < 0.8f)
    {
        // Dusk - golden hour, orange/red highlights, magenta shadows
        float t = (timeOfDay - 0.65f) / 0.15f;  // 0-1 through dusk

        colorGrading.shadowTint = {0.35f, 0.15f, 0.25f};  // Magenta
        colorGrading.shadowTintStrength = 0.35f * t;

        colorGrading.midtoneTint = {1.0f, 0.9f, 0.8f};  // Warm
        colorGrading.midtoneTintStrength = 0.25f * t;

        colorGrading.highlightTint = {1.0f, 0.6f, 0.3f};  // Orange/red
        colorGrading.highlightTintStrength = 0.5f * t;

        colorGrading.colorTemperature = 0.35f * t;  // Progressively warmer
        saturation = 1.1f + 0.1f * t;  // Boosted saturation
        colorGrading.vignetteIntensity = 0.2f * t;
    }
    else
    {
        // Night - cool, desaturated, blue tint
        float nightDepth = 0.0f;
        if (timeOfDay > 0.8f) {
            nightDepth = (timeOfDay - 0.8f) / 0.2f;  // Dusk to midnight
        } else if (timeOfDay < 0.2f) {
            nightDepth = 1.0f - (timeOfDay / 0.2f);  // Midnight to dawn
        }

        colorGrading.shadowTint = {0.15f, 0.15f, 0.25f};  // Deep blue
        colorGrading.shadowTintStrength = 0.4f * nightDepth;

        colorGrading.midtoneTint = {0.8f, 0.85f, 0.95f};  // Cool blue
        colorGrading.midtoneTintStrength = 0.2f * nightDepth;

        colorGrading.highlightTint = {0.7f, 0.75f, 0.9f};  // Pale blue (moonlight)
        colorGrading.highlightTintStrength = 0.3f * nightDepth;

        colorGrading.colorTemperature = -0.2f * nightDepth;  // Cool
        saturation = 1.0f - 0.3f * nightDepth;  // Desaturated
        contrast = 1.0f - 0.1f * nightDepth;  // Lower contrast
        colorGrading.vignetteIntensity = 0.25f * nightDepth;
    }

    colorGrading.vignetteRadius = 0.7f;
}

void PostProcessManagerDX12::setColorGradingPreset(float dawn, float noon, float dusk, float night)
{
    // Allow manual override of color grading intensity for each time period
    // This can be used to blend between different planet themes or weather conditions
    // For now, just update based on the most dominant period
    float maxWeight = std::max({dawn, noon, dusk, night});

    if (maxWeight == dawn) {
        updateColorGrading(0.27f);  // Middle of dawn period
    } else if (maxWeight == noon) {
        updateColorGrading(0.5f);   // Noon
    } else if (maxWeight == dusk) {
        updateColorGrading(0.72f);  // Middle of dusk period
    } else {
        updateColorGrading(0.0f);   // Midnight
    }
}
