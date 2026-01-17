#pragma once

// DirectX 12 headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// Forward declare the Win32 WndProc handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using Microsoft::WRL::ComPtr;

// ImGuiManager - Handles ImGui initialization, rendering, and shutdown for DirectX 12
// This class manages:
// - ImGui context creation and destruction
// - Win32 platform backend initialization
// - DirectX 12 renderer backend initialization
// - Font texture SRV allocation
// - Frame rendering to command lists
class ImGuiManager {
public:
    ImGuiManager();
    ~ImGuiManager();

    // Non-copyable, non-movable
    ImGuiManager(const ImGuiManager&) = delete;
    ImGuiManager& operator=(const ImGuiManager&) = delete;
    ImGuiManager(ImGuiManager&&) = delete;
    ImGuiManager& operator=(ImGuiManager&&) = delete;

    // Initialize ImGui with DirectX 12 backend
    // Parameters:
    //   hwnd           - Window handle for Win32 input handling
    //   device         - D3D12 device pointer
    //   commandQueue   - Command queue for texture uploads
    //   numFramesInFlight - Number of back buffers (usually 2-3)
    //   rtvFormat      - Render target format (usually DXGI_FORMAT_R8G8B8A8_UNORM)
    //   srvHeap        - SRV descriptor heap for font texture
    //   srvHeapIndex   - Index in the SRV heap to use for ImGui font texture
    bool Initialize(
        HWND hwnd,
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        int numFramesInFlight,
        DXGI_FORMAT rtvFormat,
        ID3D12DescriptorHeap* srvHeap,
        UINT srvHeapIndex
    );

    // Shutdown ImGui and release all resources
    void Shutdown();

    // Start a new ImGui frame - call at the beginning of each frame
    void BeginFrame();

    // End the ImGui frame and render to command list
    // The descriptor heap must be set on the command list before calling this
    // Parameters:
    //   commandList - Graphics command list to record ImGui draw commands
    //   srvHeap     - SRV descriptor heap (must be set on command list)
    void EndFrame(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap);

    // Process Win32 messages for ImGui input
    // Returns true if ImGui handled the message
    // Call this from your WndProc before other message handling
    static LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Check if ImGui wants to capture mouse/keyboard input
    // Use these to prevent game input when interacting with ImGui
    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

    // Check if ImGui is initialized
    bool IsInitialized() const { return m_initialized; }

private:
    // Apply dark theme with custom styling
    void ApplyStyle();

    bool m_initialized = false;
    HWND m_hwnd = nullptr;
    ID3D12Device* m_device = nullptr;
    UINT m_srvDescriptorSize = 0;
};
