#include "DX12Device.h"
#include <iostream>
#include <cassert>

// Link required libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

DX12Device::DX12Device() = default;

DX12Device::~DX12Device() {
    Shutdown();
}

void DX12Device::Initialize(HWND hwnd, UINT width, UINT height, bool vsync) {
    if (m_initialized) {
        return;
    }

    m_width = width;
    m_height = height;
    m_vsync = vsync;

    // Initialize in proper order
    EnableDebugLayer();
    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain(hwnd);
    CreateDescriptorHeaps();
    CreateRenderTargetViews();
    CreateDepthStencilView();
    CreateCommandAllocatorsAndList();
    CreateFence();

    m_initialized = true;
}

void DX12Device::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Wait for GPU to finish all pending work
    WaitForGPU();

    // Close fence event handle
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    m_initialized = false;
}

void DX12Device::EnableDebugLayer() {
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        std::cout << "[DX12] Debug layer enabled\n";

        // Enable GPU-based validation (more thorough but slower)
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1))) {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            std::cout << "[DX12] GPU-based validation enabled\n";
        }
    } else {
        std::cerr << "[DX12] Warning: Failed to enable debug layer\n";
    }

    // Enable DXGI debug
    ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    }
#endif
}

void DX12Device::CreateDevice() {
    // Create DXGI factory
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(
        CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)),
        "Failed to create DXGI factory"
    );

    // Find a suitable hardware adapter
    ComPtr<IDXGIAdapter1> adapter;
    bool adapterFound = false;

    // Try to get a high-performance adapter first (for systems with multiple GPUs)
    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(m_factory.As(&factory6))) {
        for (UINT adapterIndex = 0;
             SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                 adapterIndex,
                 DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                 IID_PPV_ARGS(&adapter)));
             ++adapterIndex) {

            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Check if this adapter supports D3D12 with feature level 12_0
            if (SUCCEEDED(D3D12CreateDevice(
                    adapter.Get(),
                    D3D_FEATURE_LEVEL_12_0,
                    _uuidof(ID3D12Device),
                    nullptr))) {
                adapterFound = true;
                std::wcout << L"[DX12] Using adapter: " << desc.Description << std::endl;
                break;
            }
        }
    }

    // Fallback: enumerate adapters manually
    if (!adapterFound) {
        for (UINT adapterIndex = 0;
             SUCCEEDED(m_factory->EnumAdapters1(adapterIndex, &adapter));
             ++adapterIndex) {

            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(
                    adapter.Get(),
                    D3D_FEATURE_LEVEL_12_0,
                    _uuidof(ID3D12Device),
                    nullptr))) {
                adapterFound = true;
                std::wcout << L"[DX12] Using adapter: " << desc.Description << std::endl;
                break;
            }
        }
    }

    if (!adapterFound) {
        throw std::runtime_error("No DirectX 12 compatible adapter found");
    }

    // Create the device with feature level 12_0
    ThrowIfFailed(
        D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&m_device)
        ),
        "Failed to create D3D12 device"
    );

    std::cout << "[DX12] Device created with feature level 12_0\n";

#if defined(_DEBUG)
    // Configure debug message filtering
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        // Optionally suppress less critical warnings
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        D3D12_MESSAGE_ID denyIds[] = {
            // Suppress messages about ClearRenderTargetView when RT is not bound
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;

        infoQueue->PushStorageFilter(&filter);
    }
#endif
}

void DX12Device::CreateCommandQueue() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.NodeMask = 0;

    ThrowIfFailed(
        m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)),
        "Failed to create command queue"
    );

    std::cout << "[DX12] Command queue created\n";
}

void DX12Device::CreateSwapChain(HWND hwnd) {
    // Describe the swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = m_backBufferFormat;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    // Create swap chain for HWND
    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(
        m_factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,  // No fullscreen desc
            nullptr,  // No restrict to output
            &swapChain1
        ),
        "Failed to create swap chain"
    );

    // Disable Alt+Enter fullscreen toggle (we handle it manually)
    ThrowIfFailed(
        m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER),
        "Failed to set window association"
    );

    // Get the IDXGISwapChain3 interface
    ThrowIfFailed(
        swapChain1.As(&m_swapChain),
        "Failed to get IDXGISwapChain3 interface"
    );

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    std::cout << "[DX12] Swap chain created (" << m_width << "x" << m_height
              << ", " << FRAME_COUNT << " buffers, vsync=" << (m_vsync ? "on" : "off") << ")\n";
}

void DX12Device::CreateDescriptorHeaps() {
    // RTV (Render Target View) descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
        "Failed to create RTV descriptor heap"
    );

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // DSV (Depth Stencil View) descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
        "Failed to create DSV descriptor heap"
    );

    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // CBV/SRV/UAV descriptor heap (shader-visible)
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    cbvSrvHeapDesc.NumDescriptors = CBV_SRV_HEAP_SIZE;
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvSrvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)),
        "Failed to create CBV/SRV/UAV descriptor heap"
    );

    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_cbvSrvNextFreeIndex = 0;

    std::cout << "[DX12] Descriptor heaps created (RTV: " << FRAME_COUNT
              << ", DSV: 1, CBV/SRV: " << CBV_SRV_HEAP_SIZE << ")\n";
}

void DX12Device::CreateRenderTargetViews() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < FRAME_COUNT; i++) {
        ThrowIfFailed(
            m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
            "Failed to get swap chain buffer"
        );

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;

        // Set debug name
#if defined(_DEBUG)
        wchar_t name[32];
        swprintf_s(name, L"RenderTarget[%u]", i);
        m_renderTargets[i]->SetName(name);
#endif
    }

    std::cout << "[DX12] Render target views created\n";
}

void DX12Device::CreateDepthStencilView() {
    // Describe the depth/stencil buffer
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = m_depthStencilFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = m_depthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    ThrowIfFailed(
        m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_depthStencil)
        ),
        "Failed to create depth stencil buffer"
    );

#if defined(_DEBUG)
    m_depthStencil->SetName(L"DepthStencilBuffer");
#endif

    // Create the DSV
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = m_depthStencilFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    m_device->CreateDepthStencilView(
        m_depthStencil.Get(),
        &dsvDesc,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    std::cout << "[DX12] Depth stencil view created (" << m_width << "x" << m_height << ")\n";
}

void DX12Device::CreateCommandAllocatorsAndList() {
    // Create command allocators (one per frame)
    for (UINT i = 0; i < FRAME_COUNT; i++) {
        ThrowIfFailed(
            m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_commandAllocators[i])
            ),
            "Failed to create command allocator"
        );

#if defined(_DEBUG)
        wchar_t name[32];
        swprintf_s(name, L"CommandAllocator[%u]", i);
        m_commandAllocators[i]->SetName(name);
#endif
    }

    // Create the command list (uses allocator 0 initially)
    ThrowIfFailed(
        m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocators[0].Get(),
            nullptr,  // No initial pipeline state
            IID_PPV_ARGS(&m_commandList)
        ),
        "Failed to create command list"
    );

    // Command lists are created in the recording state, but we want to close it
    // so we can reset it at the beginning of each frame
    ThrowIfFailed(
        m_commandList->Close(),
        "Failed to close command list"
    );

    std::cout << "[DX12] Command allocators and list created\n";
}

void DX12Device::CreateFence() {
    ThrowIfFailed(
        m_device->CreateFence(
            m_fenceValues[m_frameIndex],
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&m_fence)
        ),
        "Failed to create fence"
    );

    m_fenceValues[m_frameIndex]++;

    // Create an event handle for fence synchronization
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Failed to create fence event");
    }

    std::cout << "[DX12] Fence created for frame synchronization\n";
}

void DX12Device::BeginFrame() {
    // Reset the command allocator for this frame
    ThrowIfFailed(
        m_commandAllocators[m_frameIndex]->Reset(),
        "Failed to reset command allocator"
    );

    // Reset the command list
    ThrowIfFailed(
        m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr),
        "Failed to reset command list"
    );

    // Transition the render target from present to render target state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrier);

    // Set the viewport and scissor rect
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(m_width);
    scissorRect.bottom = static_cast<LONG>(m_height);

    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    // Set the render target
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRTVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetDSVHandle();
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Clear the render target and depth buffer
    const float clearColor[] = { 0.2f, 0.3f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_cbvSrvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
}

void DX12Device::EndFrame() {
    // Transition the render target from render target to present state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrier);

    // Close the command list
    ThrowIfFailed(
        m_commandList->Close(),
        "Failed to close command list"
    );

    // Execute the command list
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // Present the frame
    UINT syncInterval = m_vsync ? 1 : 0;
    UINT presentFlags = m_vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;

    HRESULT hr = m_swapChain->Present(syncInterval, presentFlags);

    // Handle device removal
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#if defined(_DEBUG)
        HRESULT reason = (hr == DXGI_ERROR_DEVICE_REMOVED) ?
            m_device->GetDeviceRemovedReason() : hr;
        std::cerr << "[DX12] Device removed! Reason: 0x" << std::hex << reason << std::dec << "\n";
#endif
        throw std::runtime_error("GPU device removed");
    }

    ThrowIfFailed(hr, "Failed to present");

    // Move to the next frame
    MoveToNextFrame();
}

void DX12Device::MoveToNextFrame() {
    // Schedule a Signal command in the queue
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    ThrowIfFailed(
        m_commandQueue->Signal(m_fence.Get(), currentFenceValue),
        "Failed to signal fence"
    );

    // Update the frame index
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
        ThrowIfFailed(
            m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent),
            "Failed to set event on completion"
        );
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame
    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void DX12Device::WaitForGPU() {
    // Schedule a Signal command in the queue
    ThrowIfFailed(
        m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]),
        "Failed to signal fence for WaitForGPU"
    );

    // Wait until the fence has been processed
    ThrowIfFailed(
        m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent),
        "Failed to set event on completion for WaitForGPU"
    );
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame
    m_fenceValues[m_frameIndex]++;
}

void DX12Device::OnResize(UINT width, UINT height) {
    if (width == 0 || height == 0) {
        return;
    }

    if (width == m_width && height == m_height) {
        return;
    }

    // Wait for GPU to finish
    WaitForGPU();

    // Release resources that reference the swap chain
    for (UINT i = 0; i < FRAME_COUNT; i++) {
        m_renderTargets[i].Reset();
        m_fenceValues[i] = m_fenceValues[m_frameIndex];
    }
    m_depthStencil.Reset();

    // Resize the swap chain
    DXGI_SWAP_CHAIN_DESC1 desc;
    m_swapChain->GetDesc1(&desc);

    ThrowIfFailed(
        m_swapChain->ResizeBuffers(
            FRAME_COUNT,
            width,
            height,
            desc.Format,
            desc.Flags
        ),
        "Failed to resize swap chain buffers"
    );

    m_width = width;
    m_height = height;
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Recreate the render target views and depth stencil
    CreateRenderTargetViews();
    CreateDepthStencilView();

    std::cout << "[DX12] Resized to " << m_width << "x" << m_height << "\n";
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::GetCurrentRTVHandle() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Device::GetDSVHandle() const {
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Device::GetCBVSRVGPUHandle(UINT index) const {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * m_cbvSrvDescriptorSize;
    return handle;
}

UINT DX12Device::AllocateCBVSRVDescriptor() {
    if (m_cbvSrvNextFreeIndex >= CBV_SRV_HEAP_SIZE) {
        throw std::runtime_error("CBV/SRV descriptor heap is full");
    }
    return m_cbvSrvNextFreeIndex++;
}
