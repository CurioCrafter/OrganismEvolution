// GPUSteeringCompute.cpp
// GPU compute shader pipeline for creature AI steering behaviors
// Processes up to 65,536 creatures in parallel using DirectX 12 compute shaders

#include "GPUSteeringCompute.h"

// Support both standalone DX12Device and Forge Engine adapter
#ifdef USE_FORGE_ENGINE
#include "DX12DeviceAdapter.h"
#else
#include "../graphics/DX12Device.h"
#endif

#include <d3dcompiler.h>
#include <fstream>
#include <stdexcept>

// Undefine Windows min/max macros that conflict with std::min/std::max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#pragma comment(lib, "d3dcompiler.lib")

// ============================================================================
// Constructor / Destructor
// ============================================================================

GPUSteeringCompute::GPUSteeringCompute() = default;

GPUSteeringCompute::~GPUSteeringCompute() {
    Shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool GPUSteeringCompute::Initialize(GPUSteeringDeviceType* device) {
    if (m_initialized) {
        OutputDebugStringA("[GPUSteeringCompute] Already initialized\n");
        return true;
    }

    if (!device) {
        OutputDebugStringA("[GPUSteeringCompute] ERROR: device is null\n");
        return false;
    }

    if (!device->GetDevice()) {
        OutputDebugStringA("[GPUSteeringCompute] ERROR: device->GetDevice() is null\n");
        return false;
    }

    m_device = device;
    OutputDebugStringA("[GPUSteeringCompute] Device acquired successfully\n");

    // Create pipeline components in order
    OutputDebugStringA("[GPUSteeringCompute] Creating descriptor heap...\n");
    if (!CreateDescriptorHeap()) {
        OutputDebugStringA("[GPUSteeringCompute] ERROR: CreateDescriptorHeap() failed\n");
        return false;
    }
    OutputDebugStringA("[GPUSteeringCompute] Descriptor heap created\n");

    OutputDebugStringA("[GPUSteeringCompute] Creating root signature...\n");
    if (!CreateRootSignature()) {
        OutputDebugStringA("[GPUSteeringCompute] ERROR: CreateRootSignature() failed\n");
        return false;
    }
    OutputDebugStringA("[GPUSteeringCompute] Root signature created\n");

    OutputDebugStringA("[GPUSteeringCompute] Creating compute PSO...\n");
    if (!CreateComputePSO()) {
        OutputDebugStringA("[GPUSteeringCompute] ERROR: CreateComputePSO() failed - shader compilation issue?\n");
        return false;
    }
    OutputDebugStringA("[GPUSteeringCompute] Compute PSO created\n");

    OutputDebugStringA("[GPUSteeringCompute] Creating buffers...\n");
    if (!CreateBuffers()) {
        OutputDebugStringA("[GPUSteeringCompute] ERROR: CreateBuffers() failed\n");
        return false;
    }
    OutputDebugStringA("[GPUSteeringCompute] Buffers created\n");

    m_initialized = true;
    OutputDebugStringA("[GPUSteeringCompute] Initialization COMPLETE\n");
    return true;
}

void GPUSteeringCompute::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Unmap constants buffer
    if (m_constantsBuffer && m_constantsMapped) {
        m_constantsBuffer->Unmap(0, nullptr);
        m_constantsMapped = nullptr;
    }

    // Release resources (ComPtr handles release automatically)
    m_steeringReadbackBuffer.Reset();
    m_steeringOutputBuffer.Reset();
    m_foodUpload.Reset();
    m_foodBuffer.Reset();
    m_creatureInputUpload.Reset();
    m_creatureInputBuffer.Reset();
    m_constantsBuffer.Reset();
    m_descriptorHeap.Reset();
    m_computePSO.Reset();
    m_rootSignature.Reset();

    m_device = nullptr;
    m_initialized = false;
}

// ============================================================================
// Descriptor Heap Creation
// ============================================================================

bool GPUSteeringCompute::CreateDescriptorHeap() {
    ID3D12Device* d3dDevice = m_device->GetDevice();

    // Create shader-visible descriptor heap for compute resources
    // Layout: [CreatureInput SRV, Food SRV, Output UAV, Output SRV (for graphics)]
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    if (FAILED(hr)) {
        return false;
    }

    m_descriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Cache descriptor table GPU handle
    m_descriptorTableGPU = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

    return true;
}

// ============================================================================
// Root Signature Creation
// ============================================================================

bool GPUSteeringCompute::CreateRootSignature() {
    ID3D12Device* d3dDevice = m_device->GetDevice();

    // Root parameters:
    // [0] CBV - Constants buffer (b0)
    // [1] Descriptor table - SRVs (t0, t1) and UAV (u0)

    D3D12_DESCRIPTOR_RANGE1 ranges[2] = {};

    // SRV range (t0, t1) - creature inputs and food positions
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 2;
    ranges[0].BaseShaderRegister = 0;
    ranges[0].RegisterSpace = 0;
    ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
    ranges[0].OffsetInDescriptorsFromTableStart = 0;

    // UAV range (u0) - steering output
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    ranges[1].NumDescriptors = 1;
    ranges[1].BaseShaderRegister = 0;
    ranges[1].RegisterSpace = 0;
    ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    ranges[1].OffsetInDescriptorsFromTableStart = 2;  // After 2 SRVs

    D3D12_ROOT_PARAMETER1 rootParams[2] = {};

    // Root parameter 0: CBV for constants
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Root parameter 1: Descriptor table for SRVs and UAV
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 2;
    rootParams[1].DescriptorTable.pDescriptorRanges = ranges;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSigDesc.Desc_1_1.NumParameters = 2;
    rootSigDesc.Desc_1_1.pParameters = rootParams;
    rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
    rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;
    rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }

    hr = d3dDevice->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    return SUCCEEDED(hr);
}

// ============================================================================
// Compute PSO Creation
// ============================================================================

ComPtr<ID3DBlob> GPUSteeringCompute::CompileShader(const wchar_t* filename, const char* entryPoint, const char* target) {
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    // Log what we're trying to compile
    char logBuf[512];
    sprintf_s(logBuf, "[GPUSteeringCompute] Compiling shader: entry='%s' target='%s'\n", entryPoint, target);
    OutputDebugStringA(logBuf);

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT hr = D3DCompileFromFile(
        filename,
        nullptr,            // Defines
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        sprintf_s(logBuf, "[GPUSteeringCompute] SHADER COMPILATION FAILED! HRESULT=0x%08X\n", static_cast<unsigned int>(hr));
        OutputDebugStringA(logBuf);

        // Check common errors
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            OutputDebugStringA("[GPUSteeringCompute] ERROR: Shader file not found!\n");
            OutputDebugStringA("[GPUSteeringCompute] Expected: Runtime/Shaders/SteeringCompute.hlsl\n");
            OutputDebugStringA("[GPUSteeringCompute] Working directory must contain Runtime/Shaders/\n");
        } else if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
            OutputDebugStringA("[GPUSteeringCompute] ERROR: Shader path not found!\n");
        }

        if (errorBlob) {
            OutputDebugStringA("[GPUSteeringCompute] Shader compiler output:\n");
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            OutputDebugStringA("\n");
        }
        return nullptr;
    }

    sprintf_s(logBuf, "[GPUSteeringCompute] Shader compiled successfully, bytecode size: %zu bytes\n",
              shaderBlob->GetBufferSize());
    OutputDebugStringA(logBuf);

    return shaderBlob;
}

bool GPUSteeringCompute::CreateComputePSO() {
    ID3D12Device* d3dDevice = m_device->GetDevice();

    // Compile compute shader
    // Look in Runtime/Shaders relative to executable location
    ComPtr<ID3DBlob> computeShader = CompileShader(
        L"Runtime/Shaders/SteeringCompute.hlsl",
        "CSMain",
        "cs_5_1"
    );

    if (!computeShader) {
        return false;
    }

    // Create compute pipeline state
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS.pShaderBytecode = computeShader->GetBufferPointer();
    psoDesc.CS.BytecodeLength = computeShader->GetBufferSize();
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    HRESULT hr = d3dDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_computePSO));
    return SUCCEEDED(hr);
}

// ============================================================================
// Buffer Creation
// ============================================================================

bool GPUSteeringCompute::CreateBuffers() {
    ID3D12Device* d3dDevice = m_device->GetDevice();

    // Helper for creating default heap buffers
    auto CreateDefaultBuffer = [&](ComPtr<ID3D12Resource>& buffer, size_t size, D3D12_RESOURCE_FLAGS flags) -> bool {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = flags;

        HRESULT hr = d3dDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&buffer)
        );
        return SUCCEEDED(hr);
    };

    // Helper for creating upload heap buffers
    auto CreateUploadBuffer = [&](ComPtr<ID3D12Resource>& buffer, size_t size) -> bool {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = d3dDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&buffer)
        );
        return SUCCEEDED(hr);
    };

    // Helper for creating readback buffer
    auto CreateReadbackBuffer = [&](ComPtr<ID3D12Resource>& buffer, size_t size) -> bool {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = d3dDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&buffer)
        );
        return SUCCEEDED(hr);
    };

    // Calculate buffer sizes
    const size_t creatureInputSize = MAX_CREATURES * sizeof(CreatureInput);
    const size_t foodSize = MAX_FOOD_SOURCES * sizeof(FoodPosition);
    const size_t outputSize = MAX_CREATURES * sizeof(SteeringOutput);
    const size_t constantsSize = sizeof(SteeringConstants);

    // Create creature input buffer (SRV) and upload buffer
    if (!CreateDefaultBuffer(m_creatureInputBuffer, creatureInputSize, D3D12_RESOURCE_FLAG_NONE)) {
        return false;
    }
    if (!CreateUploadBuffer(m_creatureInputUpload, creatureInputSize)) {
        return false;
    }

    // Create food buffer (SRV) and upload buffer
    if (!CreateDefaultBuffer(m_foodBuffer, foodSize, D3D12_RESOURCE_FLAG_NONE)) {
        return false;
    }
    if (!CreateUploadBuffer(m_foodUpload, foodSize)) {
        return false;
    }

    // Create steering output buffer (UAV)
    if (!CreateDefaultBuffer(m_steeringOutputBuffer, outputSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)) {
        return false;
    }

    // Create readback buffer for CPU access to results
    if (!CreateReadbackBuffer(m_steeringReadbackBuffer, outputSize)) {
        return false;
    }

    // Create constants buffer (upload heap for frequent updates)
    if (!CreateUploadBuffer(m_constantsBuffer, constantsSize)) {
        return false;
    }

    // Map constants buffer for CPU write access
    D3D12_RANGE readRange = { 0, 0 };  // We don't read from this buffer
    HRESULT hr = m_constantsBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantsMapped));
    if (FAILED(hr)) {
        return false;
    }

    // Initialize constants with default values
    SteeringConstants defaults;
    memcpy(m_constantsMapped, &defaults, sizeof(SteeringConstants));

    // Create descriptor views
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // Descriptor 0: Creature input SRV
    m_creatureInputSRV = cpuHandle;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MAX_CREATURES;
    srvDesc.Buffer.StructureByteStride = sizeof(CreatureInput);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    d3dDevice->CreateShaderResourceView(m_creatureInputBuffer.Get(), &srvDesc, cpuHandle);

    // Descriptor 1: Food SRV
    cpuHandle.ptr += m_descriptorSize;
    m_foodSRV = cpuHandle;
    srvDesc.Buffer.NumElements = MAX_FOOD_SOURCES;
    srvDesc.Buffer.StructureByteStride = sizeof(FoodPosition);
    d3dDevice->CreateShaderResourceView(m_foodBuffer.Get(), &srvDesc, cpuHandle);

    // Descriptor 2: Output UAV
    cpuHandle.ptr += m_descriptorSize;
    m_outputUAV = cpuHandle;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = MAX_CREATURES;
    uavDesc.Buffer.StructureByteStride = sizeof(SteeringOutput);
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    d3dDevice->CreateUnorderedAccessView(m_steeringOutputBuffer.Get(), nullptr, &uavDesc, cpuHandle);

    // Descriptor 3: Output SRV (for graphics pipeline to read results)
    cpuHandle.ptr += m_descriptorSize;
    srvDesc.Buffer.NumElements = MAX_CREATURES;
    srvDesc.Buffer.StructureByteStride = sizeof(SteeringOutput);
    d3dDevice->CreateShaderResourceView(m_steeringOutputBuffer.Get(), &srvDesc, cpuHandle);

    // Cache GPU handle for output SRV
    m_outputSRVGPU = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    m_outputSRVGPU.ptr += m_descriptorSize * 3;

    // Initialize tracked resource states
    m_creatureInputState = D3D12_RESOURCE_STATE_COMMON;
    m_foodState = D3D12_RESOURCE_STATE_COMMON;
    m_outputState = D3D12_RESOURCE_STATE_COMMON;

    return true;
}

// ============================================================================
// Data Updates
// ============================================================================

void GPUSteeringCompute::UpdateCreatureData(const std::vector<CreatureInput>& creatures) {
    if (!m_initialized || creatures.empty()) {
        return;
    }

    m_currentCreatureCount = static_cast<uint32_t>(std::min(creatures.size(), static_cast<size_t>(MAX_CREATURES)));
    size_t dataSize = m_currentCreatureCount * sizeof(CreatureInput);

    // Map upload buffer and copy data
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    HRESULT hr = m_creatureInputUpload->Map(0, &readRange, &mapped);
    if (SUCCEEDED(hr)) {
        memcpy(mapped, creatures.data(), dataSize);
        m_creatureInputUpload->Unmap(0, nullptr);
    }
}

void GPUSteeringCompute::UpdateFoodData(const std::vector<FoodPosition>& food) {
    if (!m_initialized) {
        return;
    }

    m_currentFoodCount = static_cast<uint32_t>(std::min(food.size(), static_cast<size_t>(MAX_FOOD_SOURCES)));

    if (m_currentFoodCount == 0) {
        return;
    }

    size_t dataSize = m_currentFoodCount * sizeof(FoodPosition);

    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    HRESULT hr = m_foodUpload->Map(0, &readRange, &mapped);
    if (SUCCEEDED(hr)) {
        memcpy(mapped, food.data(), dataSize);
        m_foodUpload->Unmap(0, nullptr);
    }
}

void GPUSteeringCompute::UpdateConstants(const SteeringConstants& constants) {
    if (!m_initialized || !m_constantsMapped) {
        return;
    }

    memcpy(m_constantsMapped, &constants, sizeof(SteeringConstants));
}

// ============================================================================
// Compute Dispatch
// ============================================================================

void GPUSteeringCompute::Dispatch(ID3D12GraphicsCommandList* cmdList, uint32_t creatureCount, float deltaTime, float time) {
    if (!m_initialized || creatureCount == 0) {
        return;
    }

    creatureCount = std::min(creatureCount, MAX_CREATURES);
    m_lastDispatchCount = creatureCount;

    // Update time-varying constants
    if (m_constantsMapped) {
        m_constantsMapped->creatureCount = creatureCount;
        m_constantsMapped->foodCount = m_currentFoodCount;
        m_constantsMapped->deltaTime = deltaTime;
        m_constantsMapped->time = time;
    }

    // Transition inputs to copy destinations and upload data
    TransitionResource(cmdList, m_creatureInputBuffer.Get(),
                       m_creatureInputState, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyBufferRegion(
        m_creatureInputBuffer.Get(), 0,
        m_creatureInputUpload.Get(), 0,
        static_cast<UINT64>(creatureCount) * sizeof(CreatureInput)
    );

    if (m_currentFoodCount > 0) {
        TransitionResource(cmdList, m_foodBuffer.Get(),
                           m_foodState, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->CopyBufferRegion(
            m_foodBuffer.Get(), 0,
            m_foodUpload.Get(), 0,
            static_cast<UINT64>(m_currentFoodCount) * sizeof(FoodPosition)
        );
    }

    // Transition inputs for shader reads
    TransitionResource(cmdList, m_creatureInputBuffer.Get(),
                       m_creatureInputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionResource(cmdList, m_foodBuffer.Get(),
                       m_foodState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // Transition output for UAV writes
    TransitionResource(cmdList, m_steeringOutputBuffer.Get(),
                       m_outputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Set compute pipeline state
    cmdList->SetComputeRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_computePSO.Get());

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Set root parameters
    cmdList->SetComputeRootConstantBufferView(0, m_constantsBuffer->GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(1, m_descriptorTableGPU);

    // Calculate thread groups (round up)
    uint32_t threadGroups = (creatureCount + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;

    // Dispatch compute shader
    cmdList->Dispatch(threadGroups, 1, 1);

    // UAV barrier to ensure compute writes are complete
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = m_steeringOutputBuffer.Get();
    cmdList->ResourceBarrier(1, &uavBarrier);
}

// ============================================================================
// Synchronization Barriers
// ============================================================================

void GPUSteeringCompute::InsertPreComputeBarrier(ID3D12GraphicsCommandList* cmdList) {
    if (!m_initialized) {
        return;
    }

    // Transition output buffer to UAV state before compute
    TransitionResource(cmdList, m_steeringOutputBuffer.Get(),
                       m_outputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void GPUSteeringCompute::InsertPostComputeBarrier(ID3D12GraphicsCommandList* cmdList) {
    if (!m_initialized) {
        return;
    }

    // Transition output buffer to SRV state for graphics pipeline
    TransitionResource(cmdList, m_steeringOutputBuffer.Get(),
                       m_outputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

// ============================================================================
// CPU Readback
// ============================================================================

void GPUSteeringCompute::CopyOutputToReadback(ID3D12GraphicsCommandList* cmdList, uint32_t count) {
    if (!m_initialized || count == 0) {
        return;
    }

    count = std::min(count, MAX_CREATURES);

    TransitionResource(cmdList, m_steeringOutputBuffer.Get(),
                       m_outputState, D3D12_RESOURCE_STATE_COPY_SOURCE);

    cmdList->CopyBufferRegion(
        m_steeringReadbackBuffer.Get(), 0,
        m_steeringOutputBuffer.Get(), 0,
        static_cast<UINT64>(count) * sizeof(SteeringOutput)
    );
}

void GPUSteeringCompute::ReadbackResults(std::vector<SteeringOutput>& results, uint32_t count) {
    if (!m_initialized || count == 0) {
        results.clear();
        return;
    }

    count = std::min(count, MAX_CREATURES);
    results.resize(count);

    // Note: The caller must have scheduled a copy to the readback buffer and
    // waited for GPU completion before mapping.

    // Map readback buffer and copy results
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, count * sizeof(SteeringOutput) };
    HRESULT hr = m_steeringReadbackBuffer->Map(0, &readRange, &mapped);
    if (SUCCEEDED(hr)) {
        memcpy(results.data(), mapped, count * sizeof(SteeringOutput));
        D3D12_RANGE writeRange = { 0, 0 };  // We didn't write anything
        m_steeringReadbackBuffer->Unmap(0, &writeRange);
    }
}

// ============================================================================
// Resource State Tracking
// ============================================================================

void GPUSteeringCompute::TransitionResource(ID3D12GraphicsCommandList* cmdList,
                                            ID3D12Resource* resource,
                                            D3D12_RESOURCE_STATES& currentState,
                                            D3D12_RESOURCE_STATES newState) {
    if (currentState == newState) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = currentState;
    barrier.Transition.StateAfter = newState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmdList->ResourceBarrier(1, &barrier);
    currentState = newState;
}
