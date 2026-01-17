#pragma once

// =============================================================================
// DX12DeviceAdapter - Bridge between ForgeEngine RHI and GPU Compute Systems
// =============================================================================
// USE_FORGE_ENGINE Architecture:
//   When USE_FORGE_ENGINE=1 (default), GPU compute systems like GPUSteeringCompute
//   use this adapter class which wraps the ForgeEngine IDevice to provide the
//   ID3D12Device* pointer needed for DX12 compute shaders.
//
//   When USE_FORGE_ENGINE=0, systems use DX12Device directly.
//
// This pattern allows the AI/compute systems to work with either:
//   1. ForgeEngine's RHI abstraction (for integrated rendering)
//   2. Direct DX12 (for standalone compute testing)
// =============================================================================

#include <d3d12.h>
#include <wrl/client.h>

// Forward declaration - full definition in RHI.h
namespace Forge { namespace RHI { class IDevice; } }

// Minimal adapter that provides ID3D12Device* access for compute shaders
// Matches the interface expected by GPUSteeringCompute (only GetDevice() is used)
class DX12DeviceAdapter {
public:
    DX12DeviceAdapter() = default;
    ~DX12DeviceAdapter() = default;

    // Interface matching DX12Device class (what GPUSteeringCompute expects)
    ID3D12Device* GetDevice() const { return m_d3dDevice; }

    // Check if valid
    bool IsValid() const { return m_d3dDevice != nullptr; }

    // Direct device setter (used by main.cpp which has full IDevice access)
    void SetDevice(ID3D12Device* device) { m_d3dDevice = device; }

private:
    ID3D12Device* m_d3dDevice = nullptr;
};

// When USE_FORGE_ENGINE is defined, GPUSteeringCompute uses DX12DeviceAdapter
// The type alias is handled in GPUSteeringCompute.h via conditional forward declaration
