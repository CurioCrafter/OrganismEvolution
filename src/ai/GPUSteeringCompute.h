#pragma once

// DirectX 12 headers
#include <d3d12.h>
#include <wrl/client.h>

#include <vector>
#include <cstdint>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Forward declarations - select appropriate device type
// When using Forge Engine, GPUSteeringCompute uses DX12DeviceAdapter (which wraps IDevice)
// Otherwise, it uses the full DX12Device class directly
#ifdef USE_FORGE_ENGINE
#include "DX12DeviceAdapter.h"
using GPUSteeringDeviceType = DX12DeviceAdapter;
#else
class DX12Device;
using GPUSteeringDeviceType = DX12Device;
#endif

// ============================================================================
// Data Structures (must match HLSL SteeringCompute.hlsl)
// ============================================================================

// Input data for each creature - sent to GPU
struct alignas(16) CreatureInput {
    XMFLOAT3 position;
    float energy;

    XMFLOAT3 velocity;
    float fear;

    uint32_t type;          // CreatureType enum
    uint32_t isAlive;       // bool as uint
    float waterLevel;
    float padding;
};

// Output from compute shader - steering forces
struct alignas(16) SteeringOutput {
    XMFLOAT3 steeringForce;
    float priority;

    XMFLOAT3 targetPosition;
    uint32_t behaviorFlags;
};

// Food position data for herbivore pathfinding
struct alignas(16) FoodPosition {
    XMFLOAT3 position;
    float amount;
};

// Constants buffer - simulation parameters
struct alignas(256) SteeringConstants {
    // Steering forces
    float maxForce = 10.0f;
    float maxSpeed = 5.0f;
    float fleeDistance = 15.0f;
    float predatorAvoidanceMultiplier = 2.0f;

    // Flocking parameters
    float separationDistance = 3.0f;
    float alignmentDistance = 8.0f;
    float cohesionDistance = 10.0f;
    float separationWeight = 1.5f;

    float alignmentWeight = 1.0f;
    float cohesionWeight = 1.0f;
    float wanderRadius = 2.0f;
    float wanderDistance = 4.0f;

    float wanderJitter = 0.3f;
    float arriveSlowRadius = 5.0f;
    float pursuitPredictionTime = 1.0f;
    float waterAvoidanceDistance = 5.0f;

    // Simulation state
    uint32_t creatureCount = 0;
    uint32_t foodCount = 0;
    float deltaTime = 0.016f;
    float time = 0.0f;

    // Grid parameters (for future spatial hashing)
    XMFLOAT3 gridMin = { -500.0f, -50.0f, -500.0f };
    float gridCellSize = 10.0f;

    XMFLOAT3 gridMax = { 500.0f, 100.0f, 500.0f };
    uint32_t gridCellsX = 100;

    uint32_t gridCellsY = 15;
    uint32_t gridCellsZ = 100;
    uint32_t padding[2] = { 0, 0 };
};

// ============================================================================
// GPUSteeringCompute - Manages GPU compute pipeline for steering behaviors
// ============================================================================

class GPUSteeringCompute {
public:
    // Configuration
    static constexpr uint32_t MAX_CREATURES = 65536;
    static constexpr uint32_t MAX_FOOD_SOURCES = 4096;
    static constexpr uint32_t THREAD_GROUP_SIZE = 64;

    GPUSteeringCompute();
    ~GPUSteeringCompute();

    // Non-copyable
    GPUSteeringCompute(const GPUSteeringCompute&) = delete;
    GPUSteeringCompute& operator=(const GPUSteeringCompute&) = delete;

    // Initialization and shutdown
    bool Initialize(GPUSteeringDeviceType* device);
    void Shutdown();

    // Update creature data on GPU
    void UpdateCreatureData(const std::vector<CreatureInput>& creatures);
    void UpdateFoodData(const std::vector<FoodPosition>& food);
    void UpdateConstants(const SteeringConstants& constants);

    // Execute compute shader
    void Dispatch(ID3D12GraphicsCommandList* cmdList, uint32_t creatureCount, float deltaTime, float time);

    // Synchronization - insert barriers before/after compute
    void InsertPreComputeBarrier(ID3D12GraphicsCommandList* cmdList);
    void InsertPostComputeBarrier(ID3D12GraphicsCommandList* cmdList);

    // Copy output to readback buffer (requires GPU synchronization before mapping)
    void CopyOutputToReadback(ID3D12GraphicsCommandList* cmdList, uint32_t count);

    // Read results back to CPU (blocking - use sparingly)
    void ReadbackResults(std::vector<SteeringOutput>& results, uint32_t count);

    // Get output buffer for GPU-side consumption (no CPU readback)
    ID3D12Resource* GetOutputBuffer() const { return m_steeringOutputBuffer.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetOutputSRV() const { return m_outputSRVGPU; }

    // Statistics
    uint32_t GetLastDispatchCount() const { return m_lastDispatchCount; }
    bool IsInitialized() const { return m_initialized; }

private:
    // Pipeline creation
    bool CreateRootSignature();
    bool CreateComputePSO();
    bool CreateBuffers();
    bool CreateDescriptorHeap();

    // Shader compilation
    ComPtr<ID3DBlob> CompileShader(const wchar_t* filename, const char* entryPoint, const char* target);

    // Resource state tracking
    void TransitionResource(ID3D12GraphicsCommandList* cmdList,
                            ID3D12Resource* resource,
                            D3D12_RESOURCE_STATES& currentState,
                            D3D12_RESOURCE_STATES newState);

    // DX12 objects
    GPUSteeringDeviceType* m_device = nullptr;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_computePSO;

    // Descriptor heap for compute resources
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    uint32_t m_descriptorSize = 0;

    // Creature input buffer (SRV)
    ComPtr<ID3D12Resource> m_creatureInputBuffer;
    ComPtr<ID3D12Resource> m_creatureInputUpload;

    // Food positions buffer (SRV)
    ComPtr<ID3D12Resource> m_foodBuffer;
    ComPtr<ID3D12Resource> m_foodUpload;

    // Steering output buffer (UAV)
    ComPtr<ID3D12Resource> m_steeringOutputBuffer;
    ComPtr<ID3D12Resource> m_steeringReadbackBuffer;

    // Constants buffer (CBV)
    ComPtr<ID3D12Resource> m_constantsBuffer;
    SteeringConstants* m_constantsMapped = nullptr;

    // Descriptor handles
    D3D12_CPU_DESCRIPTOR_HANDLE m_creatureInputSRV = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_foodSRV = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_outputUAV = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_descriptorTableGPU = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_outputSRVGPU = {};

    // State tracking
    bool m_initialized = false;
    uint32_t m_lastDispatchCount = 0;
    uint32_t m_currentCreatureCount = 0;
    uint32_t m_currentFoodCount = 0;

    D3D12_RESOURCE_STATES m_creatureInputState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_foodState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_outputState = D3D12_RESOURCE_STATE_COMMON;
};
