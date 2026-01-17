#include "RHI/RHI.h"

#if FORGE_RHI_DX12

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#ifdef FORGE_DEBUG
#include <dxgidebug.h>
#endif

#include <mutex>
#include <cstdio>

// Undefine Windows macros that conflict with our names
#ifdef DeviceCapabilities
    #undef DeviceCapabilities
#endif

using Microsoft::WRL::ComPtr;

namespace Forge::RHI
{
    // ========================================================================
    // DX12 Resource Implementations
    // ========================================================================

    class DX12Buffer : public IBuffer
    {
    public:
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES currentState;
        BufferDesc desc;
        void* mappedPtr = nullptr;

        usize GetSize() const override { return desc.size; }
        BufferUsage GetUsage() const override { return desc.usage; }

        void* Map() override
        {
            // Upload heap buffers can stay persistently mapped - just return if already mapped
            if (mappedPtr)
            {
                return mappedPtr;
            }

            if (!desc.cpuAccess || !resource)
            {
                return nullptr;
            }

            D3D12_RANGE readRange = { 0, 0 };
            HRESULT hr = resource->Map(0, &readRange, &mappedPtr);
            if (FAILED(hr))
            {
                mappedPtr = nullptr;
                return nullptr;
            }

            return mappedPtr;
        }

        void Unmap() override
        {
            // Keep upload heap buffers persistently mapped for efficiency
            // D3D12 upload heap resources can stay mapped for their entire lifetime
            // This avoids GPU synchronization issues with re-mapping in-flight resources
        }

        ~DX12Buffer()
        {
            // Unmap when buffer is destroyed
            if (mappedPtr && resource)
            {
                resource->Unmap(0, nullptr);
                mappedPtr = nullptr;
            }
        }
    };

    class DX12Texture : public ITexture
    {
    public:
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
        TextureDesc desc;

        // Descriptor handles
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = { 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = { 0 };  // CPU handle for SRV
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = { 0 };  // GPU handle for shader binding
        u32 srvHeapIndex = UINT32_MAX;  // Index in SRV heap
        bool hasRTV = false;
        bool hasDSV = false;
        bool hasSRV = false;

        u32 GetWidth() const override { return desc.width; }
        u32 GetHeight() const override { return desc.height; }
        u32 GetDepth() const override { return desc.depth; }
        Format GetFormat() const override { return desc.format; }
        TextureType GetType() const override { return desc.type; }

        u64 GetGPUDescriptorHandle() const override
        {
            return hasSRV ? srvGpuHandle.ptr : 0;
        }
    };

    class DX12Shader : public IShader
    {
    public:
        std::vector<u8> bytecode;
        ShaderType shaderType;

        ShaderType GetType() const override { return shaderType; }
    };

    class DX12Pipeline : public IPipeline
    {
    public:
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> rootSignature;
        PrimitiveTopology topology;
    };

    class DX12Fence : public IFence
    {
    public:
        ComPtr<ID3D12Fence> fence;
        HANDLE event = nullptr;
        u64 currentValue = 0;

        ~DX12Fence()
        {
            if (event)
            {
                CloseHandle(event);
            }
        }

        u64 GetCompletedValue() const override
        {
            return fence->GetCompletedValue();
        }

        void Wait(u64 value) override
        {
            if (fence->GetCompletedValue() < value)
            {
                fence->SetEventOnCompletion(value, event);
                WaitForSingleObject(event, INFINITE);
            }
        }

        void Signal(u64 value) override
        {
            currentValue = value;
        }
    };

    class DX12Device;

    class DX12Swapchain : public ISwapchain
    {
    public:
        ComPtr<IDXGISwapChain3> swapchain;
        std::vector<UniquePtr<DX12Texture>> backBuffers;
        u32 currentIndex = 0;
        u32 width = 0;
        u32 height = 0;
        Format format;
        bool vsync = true;
        DX12Device* owner = nullptr;
        u32 rtvBaseIndex = 0;

        u32 GetCurrentBackBufferIndex() const override
        {
            return swapchain->GetCurrentBackBufferIndex();
        }

        ITexture* GetBackBuffer(u32 index) override
        {
            return backBuffers[index].get();
        }

        ITexture* GetCurrentBackbuffer() override
        {
            return backBuffers[swapchain->GetCurrentBackBufferIndex()].get();
        }

        u32 GetBackBufferCount() const override
        {
            return static_cast<u32>(backBuffers.size());
        }

        Format GetFormat() const override { return format; }
        u32 GetWidth() const override { return width; }
        u32 GetHeight() const override { return height; }

        void BeginFrame() override
        {
            // Wait for the previous frame to complete (handled by device)
        }

        void Present() override
        {
            swapchain->Present(vsync ? 1 : 0, 0);
        }

        void Resize(u32 newWidth, u32 newHeight) override;
        void SetVSync(bool enabled) override { vsync = enabled; }
    };

    class DX12CommandList : public ICommandList
    {
    public:
        ComPtr<ID3D12GraphicsCommandList> cmdList;
        ComPtr<ID3D12CommandAllocator> allocator;
        DX12Pipeline* currentPipeline = nullptr;
        ID3D12DescriptorHeap* srvHeap = nullptr;  // Reference to device's SRV heap

        void Begin() override
        {
            allocator->Reset();
            cmdList->Reset(allocator.Get(), nullptr);

            // Set the SRV descriptor heap for texture binding
            if (srvHeap)
            {
                ID3D12DescriptorHeap* heaps[] = { srvHeap };
                cmdList->SetDescriptorHeaps(1, heaps);
            }
        }

        void End() override
        {
            cmdList->Close();
        }

        void ResourceBarrier(IBuffer* buffer, ResourceState before, ResourceState after) override;
        void ResourceBarrier(ITexture* texture, ResourceState before, ResourceState after) override;

        void BeginRenderPass(const RenderPassDesc& desc) override;
        void EndRenderPass() override {}

        // Render targets (alternative to render pass)
        void SetRenderTargets(Span<ITexture*> renderTargets, ITexture* depthStencil = nullptr) override;
        void ClearRenderTarget(ITexture* texture, const Math::Vec4& color) override;
        void ClearDepthStencil(ITexture* texture, f32 depth, u8 stencil) override;

        void SetPipeline(IPipeline* pipeline) override;
        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;
        void BindVertexBuffer(u32 slot, IBuffer* buffer, u32 stride, u32 offset) override;
        void BindIndexBuffer(IBuffer* buffer, IndexFormat format, u32 offset) override;
        void BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset = 0) override;
        void BindTexture(u32 slot, ITexture* texture) override;

        void Draw(u32 vertexCount, u32 firstVertex) override;
        void DrawIndexed(u32 indexCount, u32 firstIndex, i32 vertexOffset) override;
        void DrawInstanced(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) override;
        void DrawIndexedInstanced(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) override;

        void Dispatch(u32 x, u32 y, u32 z) override;

        void CopyBuffer(IBuffer* src, IBuffer* dst, usize srcOffset, usize dstOffset, usize size) override;
        void CopyTexture(ITexture* src, ITexture* dst) override;
        void CopyBufferToTexture(IBuffer* src, ITexture* dst, u32 mipLevel) override;

        void BeginDebugMarker(StringView name) override {}
        void EndDebugMarker() override {}

        // Native handle access
        void* GetNativeCommandList() const override { return cmdList.Get(); }
    };

    // ========================================================================
    // DX12 Command List Pool Implementation
    // ========================================================================

    class DX12CommandListPool : public ICommandListPool
    {
    public:
        DX12CommandListPool(ID3D12Device* device, CommandListType type, u32 initialSize, u32 maxSize,
                           ID3D12DescriptorHeap* srvHeap)
            : m_device(device)
            , m_type(type)
            , m_maxSize(maxSize)
            , m_srvHeap(srvHeap)
        {
            // Determine D3D12 command list type
            switch (type)
            {
            case CommandListType::Graphics:
                m_d3dType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
            case CommandListType::Compute:
                m_d3dType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;
            case CommandListType::Copy:
                m_d3dType = D3D12_COMMAND_LIST_TYPE_COPY;
                break;
            }

            // Pre-allocate initial command lists
            for (u32 i = 0; i < initialSize; ++i)
            {
                if (!GrowPool())
                {
                    break;
                }
            }
        }

        ~DX12CommandListPool() override = default;

        ICommandList* Acquire() override
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Try to get from free list
            if (!m_freeList.empty())
            {
                ICommandList* cmdList = m_freeList.back();
                m_freeList.pop_back();
                m_acquiredCount++;
                return cmdList;
            }

            // Try to grow the pool
            if (m_allCommandLists.size() < m_maxSize)
            {
                if (GrowPool())
                {
                    ICommandList* cmdList = m_freeList.back();
                    m_freeList.pop_back();
                    m_acquiredCount++;
                    return cmdList;
                }
            }

            return nullptr;  // Pool exhausted
        }

        void Release(ICommandList* cmdList) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_freeList.push_back(cmdList);
            m_acquiredCount--;
        }

        void Reset() override
        {
            // Move all command lists back to free list
            // Note: Actual command allocator reset happens in Begin()
            m_freeList.clear();
            m_acquiredCount = 0;
            for (auto& cmdList : m_allCommandLists)
            {
                m_freeList.push_back(cmdList.get());
            }
        }

        CommandListType GetType() const override { return m_type; }
        u32 GetAcquiredCount() const override { return m_acquiredCount; }
        u32 GetTotalCount() const override { return static_cast<u32>(m_allCommandLists.size()); }

    private:
        bool GrowPool()
        {
            HRESULT hr;

            // Create command allocator
            ComPtr<ID3D12CommandAllocator> allocator;
            hr = m_device->CreateCommandAllocator(m_d3dType, IID_PPV_ARGS(&allocator));
            if (FAILED(hr))
            {
                return false;
            }

            // Create command list
            ComPtr<ID3D12GraphicsCommandList> cmdList;
            hr = m_device->CreateCommandList(0, m_d3dType, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));
            if (FAILED(hr))
            {
                return false;
            }

            // Close immediately (will be reset in Begin())
            cmdList->Close();

            // Create wrapper
            auto wrapper = std::make_unique<DX12CommandList>();
            wrapper->cmdList = cmdList;
            wrapper->allocator = allocator;
            wrapper->srvHeap = m_srvHeap;

            m_freeList.push_back(wrapper.get());
            m_allCommandLists.push_back(std::move(wrapper));

            return true;
        }

        ID3D12Device* m_device = nullptr;
        ID3D12DescriptorHeap* m_srvHeap = nullptr;
        CommandListType m_type;
        D3D12_COMMAND_LIST_TYPE m_d3dType;
        u32 m_maxSize = 64;
        u32 m_acquiredCount = 0;

        std::mutex m_mutex;
        std::vector<UniquePtr<DX12CommandList>> m_allCommandLists;
        std::vector<ICommandList*> m_freeList;
    };

    // ========================================================================
    // DX12 Device Implementation
    // ========================================================================

    class DX12Device : public IDevice
    {
    public:
        friend class DX12Swapchain;
        DX12Device(const DeviceConfig& config);
        ~DX12Device() override;

        UniquePtr<IBuffer> CreateBuffer(const BufferDesc& desc) override;
        UniquePtr<ITexture> CreateTexture(const TextureDesc& desc) override;
        UniquePtr<IShader> CreateShader(const ShaderDesc& desc) override;
        UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& desc) override;
        UniquePtr<IPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
        UniquePtr<IPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
        UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& desc) override;
        UniquePtr<ICommandList> CreateCommandList(CommandListType type) override;
        UniquePtr<IFence> CreateFence(u64 initialValue) override;
        UniquePtr<ICommandListPool> CreateCommandListPool(const CommandListPoolDesc& desc) override;

        void Submit(ICommandList* commandList) override;
        void Submit(Span<ICommandList*> commandLists) override;

        void WaitIdle() override;
        void SignalFence(IFence* fence, u64 value) override;
        void WaitFence(IFence* fence, u64 value) override;

        void BeginFrame() override;
        void EndFrame() override;
        u32 GetCurrentFrameIndex() const override { return m_frameIndex; }
        u32 GetFrameCount() const override { return m_frameCount; }

        const DeviceCapabilities& GetCapabilities() const override { return m_capabilities; }
        Backend GetBackend() const override { return Backend::DirectX12; }

        // Native handle access (for ImGui integration)
        void* GetNativeDevice() const override { return m_device.Get(); }
        void* GetNativeSRVHeap() const override { return m_cbvSrvUavHeap.Get(); }

        // DX12 specific getters
        ID3D12Device* GetD3DDevice() { return m_device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() { return m_commandQueue.Get(); }
        IDXGIFactory4* GetDXGIFactory() { return m_factory.Get(); }

    private:
        DXGI_FORMAT ConvertFormat(Format format);
        D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state);

        ComPtr<IDXGIFactory4> m_factory;
        ComPtr<IDXGIAdapter1> m_adapter;
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;

        // Descriptor heaps
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;

        u32 m_rtvDescriptorSize = 0;
        u32 m_dsvDescriptorSize = 0;
        u32 m_cbvSrvUavDescriptorSize = 0;

        // Simple linear allocators for descriptor heaps
        u32 m_nextRtvIndex = 0;
        u32 m_nextDsvIndex = 0;
        u32 m_nextSrvIndex = 0;

        // Frame management
        u32 m_frameIndex = 0;
        u32 m_frameCount = 2;
        std::vector<UniquePtr<DX12Fence>> m_frameFences;
        std::vector<u64> m_fenceValues;

        DeviceCapabilities m_capabilities;
        std::string m_deviceName;

        #ifdef FORGE_DEBUG
        ComPtr<ID3D12Debug> m_debugController;
        #endif
    };

    DX12Device::DX12Device(const DeviceConfig& config)
        : m_frameCount(config.frameBufferCount)
    {
        HRESULT hr;

        // Enable debug layer
        #ifdef FORGE_DEBUG
        if (config.enableValidation)
        {
            hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController));
            if (SUCCEEDED(hr))
            {
                m_debugController->EnableDebugLayer();

                if (config.enableGPUValidation)
                {
                    ComPtr<ID3D12Debug1> debug1;
                    if (SUCCEEDED(m_debugController.As(&debug1)))
                    {
                        debug1->SetEnableGPUBasedValidation(TRUE);
                    }
                }
            }
        }
        #endif

        // Create DXGI factory
        UINT factoryFlags = 0;
        #ifdef FORGE_DEBUG
        if (config.enableValidation)
        {
            factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }
        #endif

        hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create DXGI factory");

        // Find adapter
        for (UINT i = 0; m_factory->EnumAdapters1(i, &m_adapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            m_adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            // Try to create device
            hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
            if (SUCCEEDED(hr))
            {
                // Store device name
                char nameBuf[256];
                WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
                m_deviceName = nameBuf;

                m_capabilities.deviceName = m_deviceName;
                m_capabilities.dedicatedVideoMemory = desc.DedicatedVideoMemory;
                break;
            }
        }

        FORGE_VERIFY_MSG(m_device != nullptr, "Failed to create D3D12 device");

        // Create command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create command queue");

        // Get descriptor sizes
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Create RTV heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 64;
        hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create RTV heap");

        // Create DSV heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 16;
        hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create DSV heap");

        // Create CBV/SRV/UAV heap (shader visible for textures)
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
        cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvUavHeapDesc.NumDescriptors = 1024;  // Space for many textures
        cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = m_device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create CBV/SRV/UAV heap");

        // Create frame fences
        m_frameFences.resize(m_frameCount);
        m_fenceValues.resize(m_frameCount, 0);

        for (u32 i = 0; i < m_frameCount; ++i)
        {
            m_frameFences[i] = UniquePtr<DX12Fence>(static_cast<DX12Fence*>(CreateFence(0).release()));
        }

        // Query capabilities
        m_capabilities.maxTextureSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        m_capabilities.maxRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
        {
            m_capabilities.supportsRaytracing = options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
        {
            m_capabilities.supportsMeshShaders = options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
        }
    }

    DX12Device::~DX12Device()
    {
        WaitIdle();
    }

    UniquePtr<IBuffer> DX12Device::CreateBuffer(const BufferDesc& desc)
    {
        auto buffer = MakeUnique<DX12Buffer>();
        buffer->desc = desc;

        D3D12_HEAP_TYPE heapType = desc.cpuAccess ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = heapType;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = desc.size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_RESOURCE_STATES initialState = desc.cpuAccess ?
            D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &resourceDesc, initialState,
            nullptr, IID_PPV_ARGS(&buffer->resource));

        if (FAILED(hr))
        {
            std::fprintf(stderr,
                "[DX12Device] CreateBuffer failed (0x%08X) size=%zu usage=0x%X cpuAccess=%d name=%.*s\n",
                static_cast<unsigned int>(hr),
                static_cast<size_t>(desc.size),
                static_cast<unsigned int>(desc.usage),
                desc.cpuAccess ? 1 : 0,
                static_cast<int>(desc.debugName.size()),
                desc.debugName.data());
        }
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create buffer");

        buffer->currentState = initialState;

        return buffer;
    }

    UniquePtr<ITexture> DX12Device::CreateTexture(const TextureDesc& desc)
    {
        auto texture = MakeUnique<DX12Texture>();
        texture->desc = desc;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = (desc.type == TextureType::Texture3D) ?
            D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = desc.width;
        resourceDesc.Height = desc.height;
        resourceDesc.DepthOrArraySize = (desc.type == TextureType::Texture3D) ?
            static_cast<u16>(desc.depth) : static_cast<u16>(desc.arraySize);
        resourceDesc.MipLevels = static_cast<u16>(desc.mipLevels);
        resourceDesc.Format = ConvertFormat(desc.format);
        resourceDesc.SampleDesc.Count = desc.sampleCount;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        if (HasFlag(desc.usage, TextureUsage::RenderTarget))
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (HasFlag(desc.usage, TextureUsage::DepthStencil))
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if (HasFlag(desc.usage, TextureUsage::UnorderedAccess))
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_CLEAR_VALUE* clearValuePtr = nullptr;
        D3D12_CLEAR_VALUE cv = {};

        if (HasFlag(desc.usage, TextureUsage::RenderTarget))
        {
            cv.Format = resourceDesc.Format;
            cv.Color[0] = desc.clearColor.x;
            cv.Color[1] = desc.clearColor.y;
            cv.Color[2] = desc.clearColor.z;
            cv.Color[3] = desc.clearColor.w;
            clearValuePtr = &cv;
        }
        else if (HasFlag(desc.usage, TextureUsage::DepthStencil))
        {
            cv.Format = resourceDesc.Format;
            cv.DepthStencil.Depth = desc.clearDepth;
            cv.DepthStencil.Stencil = desc.clearStencil;
            clearValuePtr = &cv;
        }

        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
            clearValuePtr, IID_PPV_ARGS(&texture->resource));

        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create texture");

        texture->currentState = D3D12_RESOURCE_STATE_COMMON;

        // Create DSV for depth stencil textures
        if (HasFlag(desc.usage, TextureUsage::DepthStencil))
        {
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
            dsvHandle.ptr += static_cast<SIZE_T>(m_nextDsvIndex) * m_dsvDescriptorSize;
            m_nextDsvIndex++;

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = resourceDesc.Format;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;

            m_device->CreateDepthStencilView(texture->resource.Get(), &dsvDesc, dsvHandle);
            texture->dsvHandle = dsvHandle;
            texture->hasDSV = true;
        }

        // Create RTV for render target textures
        if (HasFlag(desc.usage, TextureUsage::RenderTarget))
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += static_cast<SIZE_T>(m_nextRtvIndex) * m_rtvDescriptorSize;
            m_nextRtvIndex++;

            m_device->CreateRenderTargetView(texture->resource.Get(), nullptr, rtvHandle);
            texture->rtvHandle = rtvHandle;
            texture->hasRTV = true;
        }

        // Create SRV for shader resource textures (sampling in shaders)
        if (HasFlag(desc.usage, TextureUsage::ShaderResource))
        {
            D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
            srvCpuHandle.ptr += static_cast<SIZE_T>(m_nextSrvIndex) * m_cbvSrvUavDescriptorSize;

            D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
            srvGpuHandle.ptr += static_cast<SIZE_T>(m_nextSrvIndex) * m_cbvSrvUavDescriptorSize;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = desc.mipLevels;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

            m_device->CreateShaderResourceView(texture->resource.Get(), &srvDesc, srvCpuHandle);
            texture->srvCpuHandle = srvCpuHandle;
            texture->srvGpuHandle = srvGpuHandle;
            texture->srvHeapIndex = m_nextSrvIndex;
            texture->hasSRV = true;
            m_nextSrvIndex++;
        }

        return texture;
    }

    UniquePtr<IShader> DX12Device::CreateShader(const ShaderDesc& desc)
    {
        auto shader = MakeUnique<DX12Shader>();
        shader->shaderType = desc.type;

        // If bytecode provided, use it directly
        if (!desc.bytecode.empty())
        {
            shader->bytecode.assign(desc.bytecode.begin(), desc.bytecode.end());
            return shader;
        }

        // Otherwise compile from source
        if (!desc.source.empty())
        {
            const char* target = nullptr;
            switch (desc.type)
            {
                case ShaderType::Vertex:   target = "vs_5_0"; break;
                case ShaderType::Pixel:    target = "ps_5_0"; break;
                case ShaderType::Geometry: target = "gs_5_0"; break;
                case ShaderType::Hull:     target = "hs_5_0"; break;
                case ShaderType::Domain:   target = "ds_5_0"; break;
                case ShaderType::Compute:  target = "cs_5_0"; break;
                default: target = "vs_5_0"; break;
            }

            UINT compileFlags = 0;
            #ifdef FORGE_DEBUG
            compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
            #endif

            ComPtr<ID3DBlob> shaderBlob;
            ComPtr<ID3DBlob> errorBlob;

            std::string entryPoint(desc.entryPoint.data(), desc.entryPoint.size());
            std::string source(desc.source.data(), desc.source.size());

            HRESULT hr = D3DCompile(
                source.c_str(),
                source.size(),
                nullptr,  // source name
                nullptr,  // defines
                nullptr,  // include handler
                entryPoint.c_str(),
                target,
                compileFlags,
                0,
                &shaderBlob,
                &errorBlob);

            if (FAILED(hr))
            {
                if (errorBlob)
                {
                    OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
                    // Also print to console for visibility
                    printf("SHADER COMPILATION ERROR:\n%s\n", static_cast<const char*>(errorBlob->GetBufferPointer()));
                    fflush(stdout);
                }
                FORGE_VERIFY_MSG(false, "Shader compilation failed");
                return nullptr;
            }

            shader->bytecode.resize(shaderBlob->GetBufferSize());
            memcpy(shader->bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
        }

        return shader;
    }

    UniquePtr<IPipeline> DX12Device::CreatePipeline(const PipelineDesc& desc)
    {
        auto pipeline = MakeUnique<DX12Pipeline>();
        pipeline->topology = desc.primitiveTopology;

        // Create root signature with:
        // - Slot 0: CBV for vertex shader constants (b0)
        // - Slot 1: Descriptor table for SRV (t0) - textures
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0;  // t0
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParams[3] = {};
        // Param 0: CBV for constants (register b0)
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;  // b0
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // Param 1: CBV for per-object constants (register b1)
        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[1].Descriptor.ShaderRegister = 1;  // b1
        rootParams[1].Descriptor.RegisterSpace = 0;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // Param 2: Descriptor table for textures
        rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
        rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Static sampler for texture sampling
        D3D12_STATIC_SAMPLER_DESC staticSampler = {};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.MipLODBias = 0;
        staticSampler.MaxAnisotropy = 1;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSampler.MinLOD = 0.0f;
        staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        staticSampler.ShaderRegister = 0;  // s0
        staticSampler.RegisterSpace = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 3;
        rootSigDesc.pParameters = rootParams;
        rootSigDesc.NumStaticSamplers = 1;
        rootSigDesc.pStaticSamplers = &staticSampler;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr) && error)
        {
            OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
        }

        m_device->CreateRootSignature(0, signature->GetBufferPointer(),
                                      signature->GetBufferSize(),
                                      IID_PPV_ARGS(&pipeline->rootSignature));

        // Create PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = pipeline->rootSignature.Get();

        if (desc.vertexShader)
        {
            auto* vs = static_cast<DX12Shader*>(desc.vertexShader);
            psoDesc.VS = { vs->bytecode.data(), vs->bytecode.size() };
        }

        if (desc.pixelShader)
        {
            auto* ps = static_cast<DX12Shader*>(desc.pixelShader);
            psoDesc.PS = { ps->bytecode.data(), ps->bytecode.size() };
        }

        // Input layout
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
        for (const auto& elem : desc.vertexLayout)
        {
            D3D12_INPUT_ELEMENT_DESC d3dElem = {};
            d3dElem.SemanticName = elem.semanticName.data();
            d3dElem.SemanticIndex = elem.semanticIndex;
            d3dElem.Format = ConvertFormat(elem.format);
            d3dElem.InputSlot = elem.inputSlot;
            // Use explicit offset - offset 0 is valid for POSITION attribute
            d3dElem.AlignedByteOffset = elem.offset;
            d3dElem.InputSlotClass = elem.inputRate == InputRate::PerInstance ?
                D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA :
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            d3dElem.InstanceDataStepRate = elem.instanceStepRate;
            inputElements.push_back(d3dElem);
        }

        psoDesc.InputLayout = { inputElements.data(), static_cast<UINT>(inputElements.size()) };

        // Rasterizer state
        psoDesc.RasterizerState.FillMode = desc.fillMode == FillMode::Solid ?
            D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
        psoDesc.RasterizerState.CullMode = desc.cullMode == CullMode::None ?
            D3D12_CULL_MODE_NONE : (desc.cullMode == CullMode::Front ?
            D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK);
        psoDesc.RasterizerState.FrontCounterClockwise = desc.frontFace == FrontFace::CounterClockwise;
        psoDesc.RasterizerState.DepthBias = desc.depthBias;
        psoDesc.RasterizerState.DepthClipEnable = desc.depthClipEnabled;

        // Depth stencil state
        psoDesc.DepthStencilState.DepthEnable = desc.depthTestEnabled;
        psoDesc.DepthStencilState.DepthWriteMask = desc.depthWriteEnabled ?
            D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

        // Convert compare op
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
        switch (desc.depthCompareOp)
        {
            case CompareOp::Never:        depthFunc = D3D12_COMPARISON_FUNC_NEVER; break;
            case CompareOp::Less:         depthFunc = D3D12_COMPARISON_FUNC_LESS; break;
            case CompareOp::Equal:        depthFunc = D3D12_COMPARISON_FUNC_EQUAL; break;
            case CompareOp::LessEqual:    depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
            case CompareOp::Greater:      depthFunc = D3D12_COMPARISON_FUNC_GREATER; break;
            case CompareOp::NotEqual:     depthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
            case CompareOp::GreaterEqual: depthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
            case CompareOp::Always:       depthFunc = D3D12_COMPARISON_FUNC_ALWAYS; break;
        }
        psoDesc.DepthStencilState.DepthFunc = depthFunc;

        // Blend state
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = desc.blendEnabled;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        if (desc.blendEnabled)
        {
            D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
            D3D12_BLEND dstBlend = D3D12_BLEND_ZERO;
            D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;

            // Convert source blend factor
            if (desc.srcBlend == BlendFactor::Zero) srcBlend = D3D12_BLEND_ZERO;
            else if (desc.srcBlend == BlendFactor::One) srcBlend = D3D12_BLEND_ONE;
            else if (desc.srcBlend == BlendFactor::SrcColor) srcBlend = D3D12_BLEND_SRC_COLOR;
            else if (desc.srcBlend == BlendFactor::InvSrcColor) srcBlend = D3D12_BLEND_INV_SRC_COLOR;
            else if (desc.srcBlend == BlendFactor::SrcAlpha) srcBlend = D3D12_BLEND_SRC_ALPHA;
            else if (desc.srcBlend == BlendFactor::InvSrcAlpha) srcBlend = D3D12_BLEND_INV_SRC_ALPHA;
            else if (desc.srcBlend == BlendFactor::DstAlpha) srcBlend = D3D12_BLEND_DEST_ALPHA;
            else if (desc.srcBlend == BlendFactor::InvDstAlpha) srcBlend = D3D12_BLEND_INV_DEST_ALPHA;
            else if (desc.srcBlend == BlendFactor::DstColor) srcBlend = D3D12_BLEND_DEST_COLOR;
            else if (desc.srcBlend == BlendFactor::InvDstColor) srcBlend = D3D12_BLEND_INV_DEST_COLOR;

            // Convert dest blend factor
            if (desc.dstBlend == BlendFactor::Zero) dstBlend = D3D12_BLEND_ZERO;
            else if (desc.dstBlend == BlendFactor::One) dstBlend = D3D12_BLEND_ONE;
            else if (desc.dstBlend == BlendFactor::SrcColor) dstBlend = D3D12_BLEND_SRC_COLOR;
            else if (desc.dstBlend == BlendFactor::InvSrcColor) dstBlend = D3D12_BLEND_INV_SRC_COLOR;
            else if (desc.dstBlend == BlendFactor::SrcAlpha) dstBlend = D3D12_BLEND_SRC_ALPHA;
            else if (desc.dstBlend == BlendFactor::InvSrcAlpha) dstBlend = D3D12_BLEND_INV_SRC_ALPHA;
            else if (desc.dstBlend == BlendFactor::DstAlpha) dstBlend = D3D12_BLEND_DEST_ALPHA;
            else if (desc.dstBlend == BlendFactor::InvDstAlpha) dstBlend = D3D12_BLEND_INV_DEST_ALPHA;
            else if (desc.dstBlend == BlendFactor::DstColor) dstBlend = D3D12_BLEND_DEST_COLOR;
            else if (desc.dstBlend == BlendFactor::InvDstColor) dstBlend = D3D12_BLEND_INV_DEST_COLOR;

            // Convert blend op
            if (desc.blendOp == BlendOp::Add) blendOp = D3D12_BLEND_OP_ADD;
            else if (desc.blendOp == BlendOp::Subtract) blendOp = D3D12_BLEND_OP_SUBTRACT;
            else if (desc.blendOp == BlendOp::RevSubtract) blendOp = D3D12_BLEND_OP_REV_SUBTRACT;
            else if (desc.blendOp == BlendOp::Min) blendOp = D3D12_BLEND_OP_MIN;
            else if (desc.blendOp == BlendOp::Max) blendOp = D3D12_BLEND_OP_MAX;

            psoDesc.BlendState.RenderTarget[0].SrcBlend = srcBlend;
            psoDesc.BlendState.RenderTarget[0].DestBlend = dstBlend;
            psoDesc.BlendState.RenderTarget[0].BlendOp = blendOp;
            psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        }

        // Output
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = static_cast<UINT>(desc.renderTargetFormats.size());

        for (usize i = 0; i < desc.renderTargetFormats.size(); ++i)
        {
            psoDesc.RTVFormats[i] = ConvertFormat(desc.renderTargetFormats[i]);
        }

        if (desc.depthStencilFormat != Format::Unknown)
        {
            psoDesc.DSVFormat = ConvertFormat(desc.depthStencilFormat);
        }

        psoDesc.SampleDesc.Count = desc.sampleCount;

        hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline->pso));
        if (FAILED(hr))
        {
            // Print detailed error info to console
            printf("CreateGraphicsPipelineState failed with HRESULT: 0x%08X\n", (unsigned)hr);
            printf("  Pipeline: %s\n", desc.debugName.empty() ? "(unnamed)" : String(desc.debugName).c_str());
            printf("  VS bytecode: %zu bytes\n", psoDesc.VS.BytecodeLength);
            printf("  PS bytecode: %zu bytes\n", psoDesc.PS.BytecodeLength);
            printf("  Input layout elements: %u\n", psoDesc.InputLayout.NumElements);
            printf("  Render targets: %u\n", psoDesc.NumRenderTargets);

            // Check device status
            HRESULT reason = m_device->GetDeviceRemovedReason();
            if (FAILED(reason))
            {
                printf("  Device removed reason: 0x%08X\n", (unsigned)reason);
            }
            fflush(stdout);
        }
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create graphics pipeline");

        return pipeline;
    }

    UniquePtr<IPipeline> DX12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        // GraphicsPipelineDesc is now an alias for PipelineDesc
        return CreatePipeline(desc);
    }

    UniquePtr<IPipeline> DX12Device::CreateComputePipeline(const ComputePipelineDesc& desc)
    {
        auto pipeline = MakeUnique<DX12Pipeline>();
        // TODO: Implement compute pipeline
        return pipeline;
    }

    UniquePtr<ISwapchain> DX12Device::CreateSwapchain(const SwapchainDesc& desc)
    {
        auto swapchain = MakeUnique<DX12Swapchain>();
        swapchain->owner = this;
        swapchain->width = desc.width;
        swapchain->height = desc.height;
        swapchain->format = desc.format;
        swapchain->vsync = desc.vsync;

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = desc.width;
        swapchainDesc.Height = desc.height;
        swapchainDesc.Format = ConvertFormat(desc.format);
        swapchainDesc.Stereo = FALSE;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = desc.bufferCount;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        ComPtr<IDXGISwapChain1> swapchain1;
        HRESULT hr = m_factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            static_cast<HWND>(desc.windowHandle),
            &swapchainDesc,
            nullptr,
            nullptr,
            &swapchain1);

        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create swapchain");

        hr = swapchain1.As(&swapchain->swapchain);
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to get IDXGISwapChain3");

        // Disable Alt+Enter fullscreen
        m_factory->MakeWindowAssociation(static_cast<HWND>(desc.windowHandle), DXGI_MWA_NO_ALT_ENTER);

        // Create back buffer textures with RTVs
        swapchain->rtvBaseIndex = m_nextRtvIndex;
        swapchain->backBuffers.resize(desc.bufferCount);
        for (u32 i = 0; i < desc.bufferCount; ++i)
        {
            auto texture = MakeUnique<DX12Texture>();
            texture->desc.width = desc.width;
            texture->desc.height = desc.height;
            texture->desc.format = desc.format;
            texture->desc.type = TextureType::Texture2D;

            hr = swapchain->swapchain->GetBuffer(i, IID_PPV_ARGS(&texture->resource));
            FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to get swapchain buffer");

            texture->currentState = D3D12_RESOURCE_STATE_PRESENT;

            // Create RTV for backbuffer
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += static_cast<SIZE_T>(swapchain->rtvBaseIndex + i) * m_rtvDescriptorSize;

            m_device->CreateRenderTargetView(texture->resource.Get(), nullptr, rtvHandle);
            texture->rtvHandle = rtvHandle;
            texture->hasRTV = true;

            swapchain->backBuffers[i] = std::move(texture);
        }
        m_nextRtvIndex += desc.bufferCount;

        return swapchain;
    }

    UniquePtr<ICommandList> DX12Device::CreateCommandList(CommandListType type)
    {
        auto cmdList = MakeUnique<DX12CommandList>();

        D3D12_COMMAND_LIST_TYPE d3dType;
        switch (type)
        {
            case CommandListType::Graphics: d3dType = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
            case CommandListType::Compute:  d3dType = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
            case CommandListType::Copy:     d3dType = D3D12_COMMAND_LIST_TYPE_COPY; break;
            default:                        d3dType = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
        }

        HRESULT hr = m_device->CreateCommandAllocator(d3dType, IID_PPV_ARGS(&cmdList->allocator));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create command allocator");

        hr = m_device->CreateCommandList(0, d3dType, cmdList->allocator.Get(),
                                         nullptr, IID_PPV_ARGS(&cmdList->cmdList));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create command list");

        // Store reference to SRV heap for texture binding
        cmdList->srvHeap = m_cbvSrvUavHeap.Get();

        cmdList->cmdList->Close();

        return cmdList;
    }

    UniquePtr<IFence> DX12Device::CreateFence(u64 initialValue)
    {
        auto fence = MakeUnique<DX12Fence>();
        fence->currentValue = initialValue;

        HRESULT hr = m_device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->fence));
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to create fence");

        fence->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        FORGE_VERIFY_MSG(fence->event != nullptr, "Failed to create fence event");

        return fence;
    }

    UniquePtr<ICommandListPool> DX12Device::CreateCommandListPool(const CommandListPoolDesc& desc)
    {
        return MakeUnique<DX12CommandListPool>(
            m_device.Get(),
            desc.type,
            desc.initialPoolSize,
            desc.maxPoolSize,
            m_cbvSrvUavHeap.Get()
        );
    }

    void DX12Device::Submit(ICommandList* commandList)
    {
        auto* dx12CmdList = static_cast<DX12CommandList*>(commandList);
        ID3D12CommandList* cmdLists[] = { dx12CmdList->cmdList.Get() };
        m_commandQueue->ExecuteCommandLists(1, cmdLists);
    }

    void DX12Device::Submit(Span<ICommandList*> commandLists)
    {
        std::vector<ID3D12CommandList*> cmdLists;
        cmdLists.reserve(commandLists.size());

        for (ICommandList* cmdList : commandLists)
        {
            auto* dx12CmdList = static_cast<DX12CommandList*>(cmdList);
            cmdLists.push_back(dx12CmdList->cmdList.Get());
        }

        m_commandQueue->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());
    }

    void DX12Device::WaitIdle()
    {
        auto fence = CreateFence(0);
        auto* dx12Fence = static_cast<DX12Fence*>(fence.get());

        m_commandQueue->Signal(dx12Fence->fence.Get(), 1);
        dx12Fence->Wait(1);
    }

    void DX12Device::SignalFence(IFence* fence, u64 value)
    {
        auto* dx12Fence = static_cast<DX12Fence*>(fence);
        m_commandQueue->Signal(dx12Fence->fence.Get(), value);
        dx12Fence->currentValue = value;
    }

    void DX12Device::WaitFence(IFence* fence, u64 value)
    {
        auto* dx12Fence = static_cast<DX12Fence*>(fence);
        dx12Fence->Wait(value);
    }

    void DX12Device::BeginFrame()
    {
        // Wait for this frame's fence
        m_frameFences[m_frameIndex]->Wait(m_fenceValues[m_frameIndex]);
    }

    void DX12Device::EndFrame()
    {
        // Signal frame fence
        m_fenceValues[m_frameIndex]++;
        m_commandQueue->Signal(m_frameFences[m_frameIndex]->fence.Get(), m_fenceValues[m_frameIndex]);

        // Advance frame
        m_frameIndex = (m_frameIndex + 1) % m_frameCount;
    }

    DXGI_FORMAT DX12Device::ConvertFormat(Format format)
    {
        switch (format)
        {
            case Format::R8_UNORM:           return DXGI_FORMAT_R8_UNORM;
            case Format::R8G8_UNORM:         return DXGI_FORMAT_R8G8_UNORM;
            case Format::R8G8B8A8_UNORM:     return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::R8G8B8A8_SRGB:      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case Format::B8G8R8A8_UNORM:     return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::B8G8R8A8_SRGB:      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case Format::R16_FLOAT:          return DXGI_FORMAT_R16_FLOAT;
            case Format::R16G16_FLOAT:       return DXGI_FORMAT_R16G16_FLOAT;
            case Format::R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::R32_FLOAT:          return DXGI_FORMAT_R32_FLOAT;
            case Format::R32G32_FLOAT:       return DXGI_FORMAT_R32G32_FLOAT;
            case Format::R32G32B32_FLOAT:    return DXGI_FORMAT_R32G32B32_FLOAT;
            case Format::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::D16_UNORM:          return DXGI_FORMAT_D16_UNORM;
            case Format::D32_FLOAT:          return DXGI_FORMAT_D32_FLOAT;
            case Format::D24_UNORM_S8_UINT:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case Format::D32_FLOAT_S8_UINT:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            default:                         return DXGI_FORMAT_UNKNOWN;
        }
    }

    D3D12_RESOURCE_STATES DX12Device::ConvertResourceState(ResourceState state)
    {
        switch (state)
        {
            case ResourceState::Common:          return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::VertexBuffer:    return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case ResourceState::IndexBuffer:     return D3D12_RESOURCE_STATE_INDEX_BUFFER;
            case ResourceState::ConstantBuffer:  return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case ResourceState::ShaderResource:  return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case ResourceState::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case ResourceState::RenderTarget:    return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case ResourceState::DepthWrite:      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case ResourceState::DepthRead:       return D3D12_RESOURCE_STATE_DEPTH_READ;
            case ResourceState::CopySource:      return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case ResourceState::CopyDest:        return D3D12_RESOURCE_STATE_COPY_DEST;
            case ResourceState::Present:         return D3D12_RESOURCE_STATE_PRESENT;
            default:                             return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    // ========================================================================
    // Command List Implementation
    // ========================================================================

    void DX12CommandList::ResourceBarrier(IBuffer* buffer, ResourceState before, ResourceState after)
    {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer);

        D3D12_RESOURCE_STATES stateBefore = D3D12_RESOURCE_STATE_COMMON;
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_COMMON;

        // Convert buffer states
        auto convertBufferState = [](ResourceState state) -> D3D12_RESOURCE_STATES {
            switch (state)
            {
                case ResourceState::Common:          return D3D12_RESOURCE_STATE_COMMON;
                case ResourceState::VertexBuffer:    return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                case ResourceState::IndexBuffer:     return D3D12_RESOURCE_STATE_INDEX_BUFFER;
                case ResourceState::ConstantBuffer:  return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                case ResourceState::CopySource:      return D3D12_RESOURCE_STATE_COPY_SOURCE;
                case ResourceState::CopyDest:        return D3D12_RESOURCE_STATE_COPY_DEST;
                case ResourceState::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                default:                             return D3D12_RESOURCE_STATE_COMMON;
            }
        };

        stateBefore = convertBufferState(before);
        stateAfter = convertBufferState(after);

        // Skip if no state change needed
        if (stateBefore == stateAfter) return;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = dx12Buffer->resource.Get();
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        dx12Buffer->currentState = stateAfter;
    }

    static D3D12_RESOURCE_STATES ConvertResourceState(ResourceState state)
    {
        switch (state)
        {
            case ResourceState::Common:       return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case ResourceState::DepthWrite:   return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case ResourceState::DepthRead:    return D3D12_RESOURCE_STATE_DEPTH_READ;
            case ResourceState::ShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case ResourceState::CopyDest:     return D3D12_RESOURCE_STATE_COPY_DEST;
            case ResourceState::CopySource:   return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case ResourceState::Present:      return D3D12_RESOURCE_STATE_PRESENT;
            default:                          return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    void DX12CommandList::ResourceBarrier(ITexture* texture, ResourceState before, ResourceState after)
    {
        auto* dx12Texture = static_cast<DX12Texture*>(texture);

        D3D12_RESOURCE_STATES stateBefore = ConvertResourceState(before);
        D3D12_RESOURCE_STATES stateAfter = ConvertResourceState(after);

        // Skip if no state change needed
        if (stateBefore == stateAfter) return;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = dx12Texture->resource.Get();
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
        dx12Texture->currentState = stateAfter;
    }

    void DX12CommandList::BeginRenderPass(const RenderPassDesc& desc)
    {
        // TODO: Full render pass implementation
    }

    void DX12CommandList::SetRenderTargets(Span<ITexture*> renderTargets, ITexture* depthStencil)
    {
        // Collect RTV handles
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
        UINT numRTVs = 0;

        for (usize i = 0; i < renderTargets.size() && i < 8; ++i)
        {
            auto* dx12Texture = static_cast<DX12Texture*>(renderTargets[i]);
            if (dx12Texture && dx12Texture->hasRTV)
            {
                rtvHandles[numRTVs++] = dx12Texture->rtvHandle;
            }
        }

        // Get DSV handle if provided
        D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
        if (depthStencil)
        {
            auto* dx12Depth = static_cast<DX12Texture*>(depthStencil);
            if (dx12Depth->hasDSV)
            {
                dsvHandle = dx12Depth->dsvHandle;
                pDSV = &dsvHandle;
            }
        }

        cmdList->OMSetRenderTargets(numRTVs, numRTVs > 0 ? rtvHandles : nullptr, FALSE, pDSV);
    }

    void DX12CommandList::ClearRenderTarget(ITexture* texture, const Math::Vec4& color)
    {
        auto* dx12Texture = static_cast<DX12Texture*>(texture);
        if (!dx12Texture->hasRTV)
        {
            return; // No RTV, can't clear
        }

        FLOAT clearColor[] = { color.x, color.y, color.z, color.w };
        cmdList->ClearRenderTargetView(dx12Texture->rtvHandle, clearColor, 0, nullptr);
    }

    void DX12CommandList::ClearDepthStencil(ITexture* texture, f32 depth, u8 stencil)
    {
        auto* dx12Texture = static_cast<DX12Texture*>(texture);
        if (!dx12Texture->hasDSV)
        {
            return;
        }

        // Determine clear flags based on format
        D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;

        // Only add stencil flag if the format has a stencil component
        DXGI_FORMAT format = dx12Texture->resource->GetDesc().Format;
        if (format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
            format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
        {
            flags |= D3D12_CLEAR_FLAG_STENCIL;
        }

        cmdList->ClearDepthStencilView(
            dx12Texture->dsvHandle,
            flags,
            depth, stencil, 0, nullptr);
    }

    void DX12CommandList::SetPipeline(IPipeline* pipeline)
    {
        currentPipeline = static_cast<DX12Pipeline*>(pipeline);
        cmdList->SetPipelineState(currentPipeline->pso.Get());
        cmdList->SetGraphicsRootSignature(currentPipeline->rootSignature.Get());

        D3D12_PRIMITIVE_TOPOLOGY topology;
        switch (currentPipeline->topology)
        {
            case PrimitiveTopology::PointList:     topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
            case PrimitiveTopology::LineList:      topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
            case PrimitiveTopology::LineStrip:     topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
            case PrimitiveTopology::TriangleList:  topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
            case PrimitiveTopology::TriangleStrip: topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
            default:                               topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        }
        cmdList->IASetPrimitiveTopology(topology);
    }

    void DX12CommandList::SetViewport(const Viewport& viewport)
    {
        D3D12_VIEWPORT vp = { viewport.x, viewport.y, viewport.width, viewport.height,
                              viewport.minDepth, viewport.maxDepth };
        cmdList->RSSetViewports(1, &vp);
    }

    void DX12CommandList::SetScissor(const Scissor& scissor)
    {
        D3D12_RECT rect = { scissor.x, scissor.y,
                           static_cast<LONG>(scissor.x + scissor.width),
                           static_cast<LONG>(scissor.y + scissor.height) };
        cmdList->RSSetScissorRects(1, &rect);
    }

    void DX12CommandList::BindVertexBuffer(u32 slot, IBuffer* buffer, u32 stride, u32 offset)
    {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer);
        D3D12_VERTEX_BUFFER_VIEW view = {};
        view.BufferLocation = dx12Buffer->resource->GetGPUVirtualAddress() + offset;
        view.SizeInBytes = static_cast<UINT>(dx12Buffer->desc.size - offset);
        view.StrideInBytes = stride;
        cmdList->IASetVertexBuffers(slot, 1, &view);
    }

    void DX12CommandList::BindIndexBuffer(IBuffer* buffer, IndexFormat format, u32 offset)
    {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer);
        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = dx12Buffer->resource->GetGPUVirtualAddress() + offset;
        view.SizeInBytes = static_cast<UINT>(dx12Buffer->desc.size - offset);
        view.Format = format == IndexFormat::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        cmdList->IASetIndexBuffer(&view);
    }

    void DX12CommandList::BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset)
    {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer);
        // DX12 CBV requires 256-byte alignment for offsets
        // The caller must ensure offset is 256-byte aligned when using offsets
        cmdList->SetGraphicsRootConstantBufferView(slot, dx12Buffer->resource->GetGPUVirtualAddress() + offset);
    }

    void DX12CommandList::BindTexture(u32 slot, ITexture* texture)
    {
        auto* dx12Texture = static_cast<DX12Texture*>(texture);
        if (dx12Texture && dx12Texture->hasSRV)
        {
            // Root parameter 2 is the descriptor table for textures
            // (Root params: 0=CBV b0, 1=CBV b1, 2=SRV table)
            cmdList->SetGraphicsRootDescriptorTable(2, dx12Texture->srvGpuHandle);
        }
    }

    void DX12CommandList::Draw(u32 vertexCount, u32 firstVertex)
    {
        cmdList->DrawInstanced(vertexCount, 1, firstVertex, 0);
    }

    void DX12CommandList::DrawIndexed(u32 indexCount, u32 firstIndex, i32 vertexOffset)
    {
        cmdList->DrawIndexedInstanced(indexCount, 1, firstIndex, vertexOffset, 0);
    }

    void DX12CommandList::DrawInstanced(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
    {
        cmdList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void DX12CommandList::DrawIndexedInstanced(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
    {
        cmdList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void DX12CommandList::Dispatch(u32 x, u32 y, u32 z)
    {
        cmdList->Dispatch(x, y, z);
    }

    void DX12CommandList::CopyBuffer(IBuffer* src, IBuffer* dst, usize srcOffset, usize dstOffset, usize size)
    {
        auto* srcBuf = static_cast<DX12Buffer*>(src);
        auto* dstBuf = static_cast<DX12Buffer*>(dst);
        cmdList->CopyBufferRegion(dstBuf->resource.Get(), dstOffset,
                                   srcBuf->resource.Get(), srcOffset, size);
    }

    void DX12CommandList::CopyTexture(ITexture* src, ITexture* dst)
    {
        auto* srcTex = static_cast<DX12Texture*>(src);
        auto* dstTex = static_cast<DX12Texture*>(dst);
        cmdList->CopyResource(dstTex->resource.Get(), srcTex->resource.Get());
    }

    void DX12CommandList::CopyBufferToTexture(IBuffer* src, ITexture* dst, u32 mipLevel)
    {
        auto* srcBuffer = static_cast<DX12Buffer*>(src);
        auto* dstTexture = static_cast<DX12Texture*>(dst);

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = srcBuffer->resource.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = 0;
        srcLoc.PlacedFootprint.Footprint.Format = dstTexture->resource->GetDesc().Format;
        srcLoc.PlacedFootprint.Footprint.Width = dstTexture->desc.width;
        srcLoc.PlacedFootprint.Footprint.Height = dstTexture->desc.height;
        srcLoc.PlacedFootprint.Footprint.Depth = 1;
        // Row pitch must be aligned to 256 bytes
        u32 bytesPerPixel = 4;  // Assuming RGBA8
        srcLoc.PlacedFootprint.Footprint.RowPitch = ((dstTexture->desc.width * bytesPerPixel + 255) / 256) * 256;

        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = dstTexture->resource.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = mipLevel;

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    }

    // Swapchain resize
    void DX12Swapchain::Resize(u32 newWidth, u32 newHeight)
    {
        backBuffers.clear();

        HRESULT hr = swapchain->ResizeBuffers(0, newWidth, newHeight, DXGI_FORMAT_UNKNOWN, 0);
        FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to resize swapchain");

        // Update dimensions
        width = newWidth;
        height = newHeight;

        // Recreate back buffer textures
        DXGI_SWAP_CHAIN_DESC1 desc;
        swapchain->GetDesc1(&desc);

        backBuffers.resize(desc.BufferCount);
        for (u32 i = 0; i < desc.BufferCount; ++i)
        {
            auto texture = MakeUnique<DX12Texture>();
            texture->desc.width = newWidth;
            texture->desc.height = newHeight;
            texture->desc.format = format;
            texture->desc.type = TextureType::Texture2D;

            hr = swapchain->GetBuffer(i, IID_PPV_ARGS(&texture->resource));
            FORGE_VERIFY_MSG(SUCCEEDED(hr), "Failed to get swapchain buffer");

            texture->currentState = D3D12_RESOURCE_STATE_PRESENT;

            if (owner)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = owner->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
                rtvHandle.ptr += static_cast<SIZE_T>(rtvBaseIndex + i) * owner->m_rtvDescriptorSize;
                owner->m_device->CreateRenderTargetView(texture->resource.Get(), nullptr, rtvHandle);
                texture->rtvHandle = rtvHandle;
                texture->hasRTV = true;
            }
            backBuffers[i] = std::move(texture);
        }

        currentIndex = swapchain->GetCurrentBackBufferIndex();
    }

} // namespace Forge::RHI

namespace Forge::RHI::DX12
{
    UniquePtr<IDevice> CreateDX12Device(const DeviceConfig& config)
    {
        return MakeUnique<Forge::RHI::DX12Device>(config);
    }
}

// Factory function for RHI interface (in Forge::RHI namespace)
namespace Forge::RHI
{
    UniquePtr<IDevice> CreateDevice(const DeviceConfig& config)
    {
        return Forge::RHI::DX12::CreateDX12Device(config);
    }
}

#endif // FORGE_RHI_DX12
