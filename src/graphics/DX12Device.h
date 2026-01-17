#pragma once

// =============================================================================
// Application-Level DX12Device Wrapper
// =============================================================================
// This is the APPLICATION-LEVEL DirectX 12 device wrapper used by the main
// simulation code. It provides a simplified interface for:
//   - Window/swapchain management
//   - Frame synchronization (fence-based)
//   - Descriptor heap management
//   - Resource state tracking
//
// NOTE: This is SEPARATE from ForgeEngine/RHI/DX12/DX12Device.cpp which is
// the low-level Forge RHI abstraction. Both exist intentionally:
//   - This file: Direct DX12 access for application-level graphics
//   - ForgeEngine RHI: Abstracted GPU compute via USE_FORGE_ENGINE define
// =============================================================================

// DirectX 12 headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

// Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <cstdint>
#include <string>
#include <stdexcept>

// Use Microsoft's ComPtr for automatic reference counting
using Microsoft::WRL::ComPtr;

// Configuration constants (check for redefinition to allow coexistence with other headers)
#ifndef FRAME_COUNT_DEFINED
#define FRAME_COUNT_DEFINED
constexpr UINT FRAME_COUNT = 2;  // Double-buffered
#endif

// DX12 Device wrapper following Microsoft's HelloTriangle pattern
class DX12Device {
public:
    DX12Device();
    ~DX12Device();

    // Non-copyable, non-movable
    DX12Device(const DX12Device&) = delete;
    DX12Device& operator=(const DX12Device&) = delete;
    DX12Device(DX12Device&&) = delete;
    DX12Device& operator=(DX12Device&&) = delete;

    // Initialization
    void Initialize(HWND hwnd, UINT width, UINT height, bool vsync = true);
    void Shutdown();

    // Frame management
    void BeginFrame();
    void EndFrame();
    void WaitForGPU();

    // Resize handling
    void OnResize(UINT width, UINT height);

    // Accessors
    ID3D12Device* GetDevice() const { return m_device.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
    ID3D12DescriptorHeap* GetRTVHeap() const { return m_rtvHeap.Get(); }
    ID3D12DescriptorHeap* GetDSVHeap() const { return m_dsvHeap.Get(); }
    ID3D12DescriptorHeap* GetCBVSRVHeap() const { return m_cbvSrvHeap.Get(); }
    ID3D12Resource* GetCurrentRenderTarget() const { return m_renderTargets[m_frameIndex].Get(); }
    ID3D12Resource* GetDepthStencil() const { return m_depthStencil.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetCBVSRVGPUHandle(UINT index) const;

    UINT GetCurrentFrameIndex() const { return m_frameIndex; }
    UINT GetRTVDescriptorSize() const { return m_rtvDescriptorSize; }
    UINT GetDSVDescriptorSize() const { return m_dsvDescriptorSize; }
    UINT GetCBVSRVDescriptorSize() const { return m_cbvSrvDescriptorSize; }

    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    float GetAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }
    DXGI_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    DXGI_FORMAT GetDepthStencilFormat() const { return m_depthStencilFormat; }

    // Descriptor heap allocation (returns index)
    UINT AllocateCBVSRVDescriptor();

private:
    // Initialization helpers
    void EnableDebugLayer();
    void CreateDevice();
    void CreateCommandQueue();
    void CreateSwapChain(HWND hwnd);
    void CreateDescriptorHeaps();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();
    void CreateCommandAllocatorsAndList();
    void CreateFence();

    // Frame synchronization
    void MoveToNextFrame();

    // Pipeline objects
    ComPtr<IDXGIFactory6> m_factory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;

    // Command objects (per-frame)
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FRAME_COUNT];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // Descriptor heaps
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;

    // Resources
    ComPtr<ID3D12Resource> m_renderTargets[FRAME_COUNT];
    ComPtr<ID3D12Resource> m_depthStencil;

    // Synchronization objects
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FRAME_COUNT] = {};
    HANDLE m_fenceEvent = nullptr;

    // Descriptor sizes
    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
    UINT m_cbvSrvDescriptorSize = 0;

    // CBV/SRV heap allocation tracking
    UINT m_cbvSrvNextFreeIndex = 0;
    static constexpr UINT CBV_SRV_HEAP_SIZE = 1024;

    // Swap chain properties
    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_frameIndex = 0;
    bool m_vsync = true;

    // Formats
    DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D32_FLOAT;

    bool m_initialized = false;
};

// Helper function to check HRESULT and throw on failure
inline void ThrowIfFailed(HRESULT hr, const char* message = "DX12 Error") {
    if (FAILED(hr)) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s (HRESULT: 0x%08X)", message, static_cast<unsigned int>(hr));
        throw std::runtime_error(buffer);
    }
}
