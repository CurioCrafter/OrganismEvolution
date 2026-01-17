#include "ImGuiManager.h"
#include <cstdio>

ImGuiManager::ImGuiManager() = default;

ImGuiManager::~ImGuiManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool ImGuiManager::Initialize(
    HWND hwnd,
    ID3D12Device* device,
    ID3D12CommandQueue* commandQueue,
    int numFramesInFlight,
    DXGI_FORMAT rtvFormat,
    ID3D12DescriptorHeap* srvHeap,
    UINT srvHeapIndex
) {
    if (m_initialized) {
        return true;
    }

    if (!hwnd || !device || !commandQueue || !srvHeap) {
        printf("ImGuiManager: Invalid parameters for initialization\n");
        return false;
    }

    m_hwnd = hwnd;
    m_device = device;
    m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard navigation

    // Apply custom styling
    ApplyStyle();

    // Setup Win32 platform backend
    if (!ImGui_ImplWin32_Init(hwnd)) {
        printf("ImGuiManager: Failed to initialize Win32 backend\n");
        ImGui::DestroyContext();
        return false;
    }

    // Calculate descriptor handles for the font texture SRV
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += static_cast<SIZE_T>(srvHeapIndex) * m_srvDescriptorSize;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += static_cast<UINT64>(srvHeapIndex) * m_srvDescriptorSize;

    // Setup DX12 renderer backend using the new InitInfo struct
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device;
    initInfo.CommandQueue = commandQueue;
    initInfo.NumFramesInFlight = numFramesInFlight;
    initInfo.RTVFormat = rtvFormat;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;  // Not using depth for ImGui
    initInfo.SrvDescriptorHeap = srvHeap;

    // Use the legacy single descriptor approach (simpler for basic usage)
    initInfo.LegacySingleSrvCpuDescriptor = cpuHandle;
    initInfo.LegacySingleSrvGpuDescriptor = gpuHandle;

    if (!ImGui_ImplDX12_Init(&initInfo)) {
        printf("ImGuiManager: Failed to initialize DX12 backend\n");
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    // Build font texture and upload to GPU
    ImGui_ImplDX12_NewFrame();

    m_initialized = true;
    printf("ImGuiManager: Initialized successfully\n");
    return true;
}

void ImGuiManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    m_hwnd = nullptr;
    m_device = nullptr;
    printf("ImGuiManager: Shutdown complete\n");
}

void ImGuiManager::BeginFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap) {
    if (!m_initialized || !commandList) {
        return;
    }

    // Finish ImGui rendering
    ImGui::Render();

    // Set the descriptor heap if provided
    if (srvHeap) {
        ID3D12DescriptorHeap* heaps[] = { srvHeap };
        commandList->SetDescriptorHeaps(1, heaps);
    }

    // Record ImGui draw commands to the command list
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

LRESULT ImGuiManager::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

bool ImGuiManager::WantCaptureMouse() const {
    if (!m_initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiManager::WantCaptureKeyboard() const {
    if (!m_initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiManager::ApplyStyle() {
    // Use dark theme as base
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    // Window styling
    style.WindowRounding = 5.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.WindowMinSize = ImVec2(100, 50);

    // Frame styling
    style.FrameRounding = 3.0f;
    style.FramePadding = ImVec2(6, 4);
    style.FrameBorderSize = 0.0f;

    // Widget styling
    style.GrabRounding = 3.0f;
    style.GrabMinSize = 10.0f;
    style.ScrollbarRounding = 3.0f;
    style.ScrollbarSize = 14.0f;

    // Spacing
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    style.IndentSpacing = 20.0f;

    // Tab styling
    style.TabRounding = 4.0f;

    // Popup styling
    style.PopupRounding = 4.0f;
    style.PopupBorderSize = 1.0f;

    // Alpha for overall window transparency
    style.Alpha = 0.95f;

    // Custom color adjustments for better visibility
    ImVec4* colors = style.Colors;

    // Slightly more blue-tinted dark theme
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.95f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.28f, 0.28f, 0.35f, 0.60f);

    // Header colors (collapsing headers, menu bar)
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.28f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.38f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.26f, 0.36f, 1.00f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.08f, 0.60f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.26f, 0.34f, 1.00f);

    // Slider/scrollbar grab
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.38f, 0.48f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.42f, 0.45f, 0.56f, 1.00f);

    // Frame background (input fields, checkboxes)
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);

    // Tab colors
    colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);

    // Checkmark and selection
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.70f, 0.95f, 1.00f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.35f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.38f, 0.38f, 0.45f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.70f, 0.95f, 1.00f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.35f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.45f, 0.70f, 0.95f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.70f, 0.95f, 0.95f);

    // Plot colors for graphs
    colors[ImGuiCol_PlotLines] = ImVec4(0.45f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.65f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
}
