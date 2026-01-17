// GPUParticleSystem.cpp - GPU-based particle system for weather effects
#include "GPUParticleSystem.h"
#include "../DX12Device.h"
#include "../Camera.h"
#include "../../environment/Terrain.h"
#include <d3dcompiler.h>
#include <algorithm>
#include <cstring>

// Thread group size - must match HLSL
static constexpr uint32_t THREAD_GROUP_SIZE = 256;

GPUParticleSystem::GPUParticleSystem() = default;

GPUParticleSystem::~GPUParticleSystem() {
    shutdown();
}

bool GPUParticleSystem::initialize(DX12Device* device) {
    if (m_initialized) {
        return true;
    }

    m_device = device;
    if (!m_device) {
        return false;
    }

    if (!createBuffers()) {
        return false;
    }

    if (!createComputePipeline()) {
        return false;
    }

    if (!createRenderPipeline()) {
        return false;
    }

    if (!createBillboardMesh()) {
        return false;
    }

    m_initialized = true;
    return true;
}

void GPUParticleSystem::shutdown() {
    if (m_mappedConstantBuffer && m_constantBuffer) {
        m_constantBuffer->Unmap(0, nullptr);
        m_mappedConstantBuffer = nullptr;
    }

    m_particleBuffers[0].Reset();
    m_particleBuffers[1].Reset();
    m_particleUploadBuffer.Reset();
    m_aliveCountBuffer.Reset();
    m_aliveCountReadback.Reset();
    m_constantBuffer.Reset();
    m_billboardVertexBuffer.Reset();
    m_billboardIndexBuffer.Reset();
    m_computePSO.Reset();
    m_computeRootSig.Reset();
    m_renderPSO.Reset();
    m_renderRootSig.Reset();

    m_device = nullptr;
    m_terrain = nullptr;
    m_initialized = false;
}

void GPUParticleSystem::emit(const ParticleEmitParams& params) {
    if (m_pendingParticles.size() >= MAX_WEATHER_PARTICLES) {
        return; // Hard cap reached
    }

    WeatherParticle p = {};
    p.position = params.position;
    p.velocity = params.velocity;
    p.life = params.life;
    p.size = params.size;
    p.rotation = params.rotation;
    p.type = params.type;

    m_pendingParticles.push_back(p);
}

void GPUParticleSystem::emitBurst(const std::vector<ParticleEmitParams>& particles) {
    for (const auto& params : particles) {
        emit(params);
    }
}

void GPUParticleSystem::clearAll() {
    m_pendingParticles.clear();
    m_cpuParticleCount = 0;

    // Note: GPU buffer will be cleared on next update via simulation
}

void GPUParticleSystem::update(float deltaTime,
                               const glm::vec3& windDirection,
                               float windStrength,
                               float precipitationIntensity,
                               float precipitationType,
                               float fogDensity,
                               float groundLevel,
                               const glm::vec3& cameraPos) {
    if (!m_initialized) {
        return;
    }

    // Store weather state for rendering
    m_windDirection = glm::length(windDirection) > 0.001f ? glm::normalize(windDirection) : glm::vec3(1, 0, 0);
    m_windStrength = windStrength;
    m_precipitationIntensity = precipitationIntensity;
    m_precipitationType = precipitationType;
    m_fogDensity = fogDensity;
    m_groundLevel = groundLevel;

    // Update spawn bounds around camera
    float spawnRadius = 50.0f;
    m_boundsMin = cameraPos - glm::vec3(spawnRadius, 0.0f, spawnRadius);
    m_boundsMax = cameraPos + glm::vec3(spawnRadius, 100.0f, spawnRadius);

    // Upload pending particles
    uploadPendingParticles();

    // Note: Actual compute shader dispatch happens in render() for proper command list sequencing
}

void GPUParticleSystem::render(ID3D12GraphicsCommandList* cmdList,
                               const Camera& camera,
                               const glm::mat4& viewProjection,
                               float time,
                               float lightningIntensity,
                               const glm::vec3& lightningPos) {
    if (!m_initialized || !cmdList) {
        return;
    }

    // Update constant buffer
    updateConstantBuffer(camera, viewProjection, time, lightningIntensity, lightningPos);

    // Swap particle buffers for ping-pong simulation
    int readBuffer = m_currentBufferIndex;
    int writeBuffer = 1 - m_currentBufferIndex;

    // --- Compute Pass: Simulate particles ---
    // Set compute root signature and PSO
    cmdList->SetComputeRootSignature(m_computeRootSig.Get());
    cmdList->SetPipelineState(m_computePSO.Get());

    // Set descriptor tables
    auto cbvSrvHeap = m_device->GetCBVSRVHeap();
    ID3D12DescriptorHeap* heaps[] = { cbvSrvHeap };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Set constant buffer
    cmdList->SetComputeRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    // Set UAVs (read buffer as SRV, write buffer as UAV)
    cmdList->SetComputeRootDescriptorTable(1, m_device->GetCBVSRVGPUHandle(m_particleBufferSRVIndex[readBuffer]));
    cmdList->SetComputeRootDescriptorTable(2, m_device->GetCBVSRVGPUHandle(m_particleBufferUAVIndex[writeBuffer]));
    cmdList->SetComputeRootDescriptorTable(3, m_device->GetCBVSRVGPUHandle(m_aliveCountUAVIndex));

    // Dispatch compute shader
    uint32_t threadGroups = (MAX_WEATHER_PARTICLES + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
    cmdList->Dispatch(threadGroups, 1, 1);

    // UAV barrier before rendering
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = m_particleBuffers[writeBuffer].Get();
    cmdList->ResourceBarrier(1, &uavBarrier);

    // --- Render Pass: Draw particles ---
    cmdList->SetGraphicsRootSignature(m_renderRootSig.Get());
    cmdList->SetPipelineState(m_renderPSO.Get());

    cmdList->SetDescriptorHeaps(1, heaps);

    // Set constant buffer
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    // Set particle buffer as SRV for vertex shader
    cmdList->SetGraphicsRootDescriptorTable(1, m_device->GetCBVSRVGPUHandle(m_particleBufferSRVIndex[writeBuffer]));

    // Set primitive topology and vertex buffer
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &m_billboardVBView);
    cmdList->IASetIndexBuffer(&m_billboardIBView);

    // Draw instanced - one quad per particle
    cmdList->DrawIndexedInstanced(6, MAX_WEATHER_PARTICLES, 0, 0, 0);

    // Swap buffers for next frame
    m_currentBufferIndex = writeBuffer;
}

void GPUParticleSystem::setRainColor(const glm::vec3& color, float alpha) {
    m_rainColor = color;
    m_rainAlpha = alpha;
}

void GPUParticleSystem::setSnowColor(const glm::vec3& color, float alpha) {
    m_snowColor = color;
    m_snowAlpha = alpha;
}

void GPUParticleSystem::setSpawnBounds(const glm::vec3& min, const glm::vec3& max, float spawnHeight) {
    m_boundsMin = min;
    m_boundsMax = max;
    m_spawnHeight = spawnHeight;
}

bool GPUParticleSystem::createBuffers() {
    auto* device = m_device->GetDevice();
    if (!device) {
        return false;
    }

    // Create particle buffers (structured buffer for both SRV and UAV access)
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = sizeof(WeatherParticle) * MAX_WEATHER_PARTICLES;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    for (int i = 0; i < 2; ++i) {
        HRESULT hr = device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_particleBuffers[i]));
        if (FAILED(hr)) {
            return false;
        }
    }

    // Create upload buffer for pending particles
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_particleUploadBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // Create alive count buffer (single uint32)
    bufferDesc.Width = sizeof(uint32_t);
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_aliveCountBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // Create constant buffer (256-byte aligned)
    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = sizeof(WeatherConstantBuffer);
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.Format = DXGI_FORMAT_UNKNOWN;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // Map constant buffer
    hr = m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // Create SRV/UAV descriptors for particle buffers
    for (int i = 0; i < 2; ++i) {
        // SRV for particle buffer
        m_particleBufferSRVIndex[i] = m_device->AllocateCBVSRVDescriptor();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = MAX_WEATHER_PARTICLES;
        srvDesc.Buffer.StructureByteStride = sizeof(WeatherParticle);

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_device->GetCBVSRVHeap()->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += m_particleBufferSRVIndex[i] * m_device->GetCBVSRVDescriptorSize();
        device->CreateShaderResourceView(m_particleBuffers[i].Get(), &srvDesc, srvHandle);

        // UAV for particle buffer
        m_particleBufferUAVIndex[i] = m_device->AllocateCBVSRVDescriptor();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = MAX_WEATHER_PARTICLES;
        uavDesc.Buffer.StructureByteStride = sizeof(WeatherParticle);

        D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = m_device->GetCBVSRVHeap()->GetCPUDescriptorHandleForHeapStart();
        uavHandle.ptr += m_particleBufferUAVIndex[i] * m_device->GetCBVSRVDescriptorSize();
        device->CreateUnorderedAccessView(m_particleBuffers[i].Get(), nullptr, &uavDesc, uavHandle);
    }

    // UAV for alive count
    m_aliveCountUAVIndex = m_device->AllocateCBVSRVDescriptor();
    D3D12_UNORDERED_ACCESS_VIEW_DESC countUAVDesc = {};
    countUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    countUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    countUAVDesc.Buffer.NumElements = 1;
    countUAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    D3D12_CPU_DESCRIPTOR_HANDLE countHandle = m_device->GetCBVSRVHeap()->GetCPUDescriptorHandleForHeapStart();
    countHandle.ptr += m_aliveCountUAVIndex * m_device->GetCBVSRVDescriptorSize();
    device->CreateUnorderedAccessView(m_aliveCountBuffer.Get(), nullptr, &countUAVDesc, countHandle);

    return true;
}

bool GPUParticleSystem::createComputePipeline() {
    auto* device = m_device->GetDevice();
    if (!device) {
        return false;
    }

    // Create root signature for compute
    D3D12_ROOT_PARAMETER rootParams[4] = {};

    // CBV for weather constants
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Descriptor table for input particle buffer (SRV)
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Descriptor table for output particle buffer (UAV)
    D3D12_DESCRIPTOR_RANGE uavRange1 = {};
    uavRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange1.NumDescriptors = 1;
    uavRange1.BaseShaderRegister = 0;

    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &uavRange1;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Descriptor table for alive count (UAV)
    D3D12_DESCRIPTOR_RANGE uavRange2 = {};
    uavRange2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange2.NumDescriptors = 1;
    uavRange2.BaseShaderRegister = 1;

    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[3].DescriptorTable.pDescriptorRanges = &uavRange2;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 4;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
    if (FAILED(hr)) {
        return false;
    }

    hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_computeRootSig));
    if (FAILED(hr)) {
        return false;
    }

    // Compile compute shader from file
    ComPtr<ID3DBlob> csBlob;
    hr = D3DCompileFromFile(L"Runtime/Shaders/Weather.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "CSSimulate", "cs_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &csBlob, &errorBlob);
    if (FAILED(hr)) {
        // Shader may not exist yet - this is okay, we'll use a fallback
        return true;
    }

    // Create compute PSO
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc = {};
    computePSODesc.pRootSignature = m_computeRootSig.Get();
    computePSODesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

    hr = device->CreateComputePipelineState(&computePSODesc, IID_PPV_ARGS(&m_computePSO));
    return SUCCEEDED(hr);
}

bool GPUParticleSystem::createRenderPipeline() {
    auto* device = m_device->GetDevice();
    if (!device) {
        return false;
    }

    // Create root signature for rendering
    D3D12_ROOT_PARAMETER rootParams[2] = {};

    // CBV for weather constants
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Descriptor table for particle buffer (SRV)
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
    if (FAILED(hr)) {
        return false;
    }

    hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_renderRootSig));
    if (FAILED(hr)) {
        return false;
    }

    // Compile vertex and pixel shaders
    ComPtr<ID3DBlob> vsBlob, psBlob;
    hr = D3DCompileFromFile(L"Runtime/Shaders/Weather.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "VSMain", "vs_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        return true; // Shader may not exist yet
    }

    hr = D3DCompileFromFile(L"Runtime/Shaders/Weather.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "PSMain", "ps_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        return true;
    }

    // Input layout for billboard vertices
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Create render PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_renderRootSig.Get();
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_device->GetBackBufferFormat();
    psoDesc.DSVFormat = m_device->GetDepthStencilFormat();
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = UINT_MAX;

    // Rasterizer state
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // Billboards face camera
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = 0;
    psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // Blend state - additive blending for particles
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // Depth state - read only (particles shouldn't write to depth)
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_renderPSO));
    return SUCCEEDED(hr);
}

bool GPUParticleSystem::createBillboardMesh() {
    auto* device = m_device->GetDevice();
    if (!device) {
        return false;
    }

    // Billboard quad vertices (position + UV)
    struct BillboardVertex {
        glm::vec3 position;
        glm::vec2 texCoord;
    };

    BillboardVertex vertices[4] = {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } }
    };

    uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    // Create vertex buffer
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC vbDesc = {};
    vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbDesc.Width = sizeof(vertices);
    vbDesc.Height = 1;
    vbDesc.DepthOrArraySize = 1;
    vbDesc.MipLevels = 1;
    vbDesc.Format = DXGI_FORMAT_UNKNOWN;
    vbDesc.SampleDesc.Count = 1;
    vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_billboardVertexBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // Upload vertex data
    void* pData;
    m_billboardVertexBuffer->Map(0, nullptr, &pData);
    memcpy(pData, vertices, sizeof(vertices));
    m_billboardVertexBuffer->Unmap(0, nullptr);

    m_billboardVBView.BufferLocation = m_billboardVertexBuffer->GetGPUVirtualAddress();
    m_billboardVBView.SizeInBytes = sizeof(vertices);
    m_billboardVBView.StrideInBytes = sizeof(BillboardVertex);

    // Create index buffer
    D3D12_RESOURCE_DESC ibDesc = vbDesc;
    ibDesc.Width = sizeof(indices);

    hr = device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_billboardIndexBuffer));
    if (FAILED(hr)) {
        return false;
    }

    // Upload index data
    m_billboardIndexBuffer->Map(0, nullptr, &pData);
    memcpy(pData, indices, sizeof(indices));
    m_billboardIndexBuffer->Unmap(0, nullptr);

    m_billboardIBView.BufferLocation = m_billboardIndexBuffer->GetGPUVirtualAddress();
    m_billboardIBView.SizeInBytes = sizeof(indices);
    m_billboardIBView.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

void GPUParticleSystem::uploadPendingParticles() {
    if (m_pendingParticles.empty()) {
        return;
    }

    // Map upload buffer and copy pending particles
    void* pData;
    HRESULT hr = m_particleUploadBuffer->Map(0, nullptr, &pData);
    if (FAILED(hr)) {
        m_pendingParticles.clear();
        return;
    }

    size_t count = std::min(m_pendingParticles.size(), static_cast<size_t>(MAX_WEATHER_PARTICLES));
    memcpy(pData, m_pendingParticles.data(), count * sizeof(WeatherParticle));
    m_particleUploadBuffer->Unmap(0, nullptr);

    m_cpuParticleCount = static_cast<uint32_t>(count);
    m_pendingParticles.clear();

    // Note: Actual GPU copy would happen via command list - simplified here
}

void GPUParticleSystem::updateConstantBuffer(const Camera& camera,
                                             const glm::mat4& viewProjection,
                                             float time,
                                             float lightningIntensity,
                                             const glm::vec3& lightningPos) {
    if (!m_mappedConstantBuffer) {
        return;
    }

    // Using packed vec4 structure - pack values accordingly
    float aspectRatio = static_cast<float>(m_device->GetWidth()) / static_cast<float>(m_device->GetHeight());
    glm::mat4 viewMat = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 projMat = camera.getProjectionMatrix(aspectRatio);

    m_mappedConstantBuffer->viewProjection = viewProjection;
    m_mappedConstantBuffer->cameraPos = glm::vec4(camera.Position, time);  // xyz = pos, w = time
    m_mappedConstantBuffer->windParams = glm::vec4(m_windDirection, m_windStrength);  // xyz = dir, w = strength
    m_mappedConstantBuffer->weatherParams = glm::vec4(m_precipitationIntensity, m_precipitationType, m_fogDensity, 50.0f);  // fog height = 50
    m_mappedConstantBuffer->boundsMin = glm::vec4(m_boundsMin, m_spawnHeight);  // xyz = min, w = spawnHeight
    m_mappedConstantBuffer->boundsMax = glm::vec4(m_boundsMax, m_groundLevel);  // xyz = max, w = groundLevel
    m_mappedConstantBuffer->rainColor = glm::vec4(m_rainColor, m_rainAlpha);  // xyz = color, w = alpha
    m_mappedConstantBuffer->snowColor = glm::vec4(m_snowColor, m_snowAlpha);  // xyz = color, w = alpha
    m_mappedConstantBuffer->lightningParams = glm::vec4(lightningIntensity, lightningPos.x, lightningPos.y, lightningPos.z);  // x = intensity, yzw = pos
}
