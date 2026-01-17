#pragma once

// Forge Engine - Render Hardware Interface
// Zero-cost abstraction layer for modern graphics APIs

#include "Core/CoreMinimal.h"
#include "Math/Vector.h"

namespace Forge::RHI
{
    // ========================================================================
    // Forward Declarations
    // ========================================================================

    class IDevice;
    class ICommandList;
    class IBuffer;
    class ITexture;
    class IPipeline;
    class IShader;
    class ISwapchain;
    class IDescriptorSet;
    class IFence;
    class ISemaphore;
    class IRenderPass;
    class ICommandListPool;
    struct ParallelCommandContext;

    // ========================================================================
    // Enums
    // ========================================================================

    enum class GraphicsAPI : u8
    {
        None,
        DirectX12,
        Vulkan,
        Auto  // Pick best for platform
    };

    // Alias for compatibility
    using Backend = GraphicsAPI;

    enum class Format : u8
    {
        Unknown,

        // Color formats
        R8_UNORM,
        R8G8_UNORM,
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SRGB,
        R16_FLOAT,
        R16G16_FLOAT,
        R16G16B16A16_FLOAT,
        R32_FLOAT,
        R32G32_FLOAT,
        R32G32B32_FLOAT,
        R32G32B32A32_FLOAT,
        R11G11B10_FLOAT,
        RGB10A2_UNORM,

        // Depth formats
        D16_UNORM,
        D32_FLOAT,
        D24_UNORM_S8_UINT,
        D32_FLOAT_S8_UINT,

        // Compressed formats
        BC1_UNORM,
        BC1_SRGB,
        BC2_UNORM,
        BC2_SRGB,
        BC3_UNORM,
        BC3_SRGB,
        BC4_UNORM,
        BC5_UNORM,
        BC7_UNORM,
        BC7_SRGB,
    };

    enum class ResourceState : u8
    {
        Undefined,
        Common,
        VertexBuffer,
        IndexBuffer,
        ConstantBuffer,
        ShaderResource,
        UnorderedAccess,
        RenderTarget,
        DepthWrite,
        DepthRead,
        CopySource,
        CopyDest,
        Present,
        IndirectArgument,
    };

    enum class BufferUsage : u8
    {
        Default = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,  // Constant buffer
        Storage = 1 << 3,
        Indirect = 1 << 4,
        CopySrc = 1 << 5,
        CopyDst = 1 << 6,
    };
    FORGE_ENABLE_ENUM_FLAGS(BufferUsage)

    // Alias for compatibility
    constexpr BufferUsage BufferUsageConstant = BufferUsage::Uniform;

    enum class TextureUsage : u8
    {
        None = 0,
        ShaderResource = 1 << 0,
        RenderTarget = 1 << 1,
        DepthStencil = 1 << 2,
        UnorderedAccess = 1 << 3,
        CopySrc = 1 << 4,
        CopyDst = 1 << 5,
    };
    FORGE_ENABLE_ENUM_FLAGS(TextureUsage)

    enum class TextureType : u8
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture2DArray,
        TextureCubeArray,
    };

    enum class IndexFormat : u8
    {
        UInt16,
        UInt32,
    };

    enum class PrimitiveTopology : u8
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
    };

    enum class CullMode : u8
    {
        None,
        Front,
        Back,
    };

    enum class FrontFace : u8
    {
        CounterClockwise,
        Clockwise,
    };

    enum class FillMode : u8
    {
        Solid,
        Wireframe,
    };

    enum class CompareOp : u8
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    enum class BlendFactor : u8
    {
        Zero,
        One,
        SrcColor,
        InvSrcColor,
        SrcAlpha,
        InvSrcAlpha,
        DstColor,
        InvDstColor,
        DstAlpha,
        InvDstAlpha,
    };

    enum class BlendOp : u8
    {
        Add,
        Subtract,
        RevSubtract,
        Min,
        Max,
    };

    enum class ShaderType : u8
    {
        Vertex,
        Pixel,
        Compute,
        Geometry,
        Hull,
        Domain,
        Mesh,
        Amplification,
    };

    // Alias for compatibility
    using ShaderStage = ShaderType;

    enum class InputRate : u8
    {
        PerVertex,
        PerInstance,
    };

    enum class CommandListType : u8
    {
        Graphics,
        Compute,
        Copy,
    };

    // ========================================================================
    // Structures
    // ========================================================================

    struct Viewport
    {
        f32 x = 0;
        f32 y = 0;
        f32 width = 0;
        f32 height = 0;
        f32 minDepth = 0.0f;
        f32 maxDepth = 1.0f;
    };

    struct Scissor
    {
        i32 x = 0;
        i32 y = 0;
        u32 width = 0;
        u32 height = 0;
    };

    // Alias for compatibility
    using Rect = Scissor;

    struct ClearValue
    {
        union
        {
            struct { f32 r, g, b, a; } color;
            struct { f32 depth; u8 stencil; } depthStencil;
        };

        ClearValue() : color{0, 0, 0, 1} {}

        static ClearValue Color(f32 r, f32 g, f32 b, f32 a = 1.0f)
        {
            ClearValue cv;
            cv.color = { r, g, b, a };
            return cv;
        }

        static ClearValue DepthStencil(f32 depth = 1.0f, u8 stencil = 0)
        {
            ClearValue cv;
            cv.depthStencil = { depth, stencil };
            return cv;
        }
    };

    // ========================================================================
    // Device Configuration
    // ========================================================================

    struct DeviceConfig
    {
        GraphicsAPI api = GraphicsAPI::Auto;
        bool enableValidation = true;
        bool enableGPUValidation = false;
        u32 frameBufferCount = 2;
        u32 adapterIndex = 0;
    };

    struct DeviceCapabilities
    {
        StringView deviceName;
        u64 dedicatedVideoMemory;
        u32 maxTextureSize;
        u32 maxRenderTargets;
        bool supportsRaytracing;
        bool supportsMeshShaders;
        bool supportsVariableRateShading;
    };

    // ========================================================================
    // Buffer
    // ========================================================================

    struct BufferDesc
    {
        usize size = 0;
        BufferUsage usage = BufferUsage::Default;
        bool cpuAccess = true;  // Allow CPU read/write
        StringView debugName;
    };

    class IBuffer
    {
    public:
        virtual ~IBuffer() = default;

        [[nodiscard]] virtual usize GetSize() const = 0;
        [[nodiscard]] virtual BufferUsage GetUsage() const = 0;

        // Map for CPU access (only if cpuAccess was set)
        [[nodiscard]] virtual void* Map() = 0;
        virtual void Unmap() = 0;
    };

    // ========================================================================
    // Texture
    // ========================================================================

    struct TextureDesc
    {
        u32 width = 1;
        u32 height = 1;
        u32 depth = 1;
        u32 mipLevels = 1;
        u32 arraySize = 1;
        Format format = Format::R8G8B8A8_UNORM;
        TextureType type = TextureType::Texture2D;
        TextureUsage usage = TextureUsage::ShaderResource;
        u32 sampleCount = 1;
        Math::Vec4 clearColor = Math::Vec4(0, 0, 0, 1);
        f32 clearDepth = 1.0f;
        u8 clearStencil = 0;
        StringView debugName;
    };

    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        [[nodiscard]] virtual u32 GetWidth() const = 0;
        [[nodiscard]] virtual u32 GetHeight() const = 0;
        [[nodiscard]] virtual u32 GetDepth() const = 0;
        [[nodiscard]] virtual Format GetFormat() const = 0;
        [[nodiscard]] virtual TextureType GetType() const = 0;

        /// Get GPU descriptor handle for ImGui rendering (returns 0 if no SRV)
        [[nodiscard]] virtual u64 GetGPUDescriptorHandle() const = 0;
    };

    // ========================================================================
    // Shader
    // ========================================================================

    struct ShaderDesc
    {
        ShaderType type = ShaderType::Vertex;
        Span<const u8> bytecode;            // Pre-compiled bytecode
        StringView source;                   // OR source code to compile
        StringView entryPoint = "main";
        StringView debugName;
    };

    class IShader
    {
    public:
        virtual ~IShader() = default;

        [[nodiscard]] virtual ShaderType GetType() const = 0;
    };

    // ========================================================================
    // Pipeline
    // ========================================================================

    struct VertexAttribute
    {
        StringView semanticName;
        u32 semanticIndex = 0;
        Format format = Format::R32G32B32A32_FLOAT;
        u32 inputSlot = 0;
        u32 offset = 0;
        InputRate inputRate = InputRate::PerVertex;
        u32 instanceStepRate = 0;
    };

    // Alias for compatibility
    using InputElement = VertexAttribute;

    struct PipelineDesc
    {
        IShader* vertexShader = nullptr;
        IShader* pixelShader = nullptr;
        IShader* geometryShader = nullptr;
        IShader* hullShader = nullptr;
        IShader* domainShader = nullptr;

        Vector<VertexAttribute> vertexLayout;
        PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;

        // Rasterizer state
        FillMode fillMode = FillMode::Solid;
        CullMode cullMode = CullMode::Back;
        FrontFace frontFace = FrontFace::CounterClockwise;
        i32 depthBias = 0;
        f32 depthBiasClamp = 0.0f;
        f32 slopeScaledDepthBias = 0.0f;
        bool depthClipEnabled = true;

        // Depth/stencil state
        bool depthTestEnabled = true;
        bool depthWriteEnabled = true;
        CompareOp depthCompareOp = CompareOp::Less;
        bool stencilEnabled = false;

        // Blend state
        bool blendEnabled = false;
        BlendFactor srcBlend = BlendFactor::One;
        BlendFactor dstBlend = BlendFactor::Zero;
        BlendOp blendOp = BlendOp::Add;

        // Render targets
        Vector<Format> renderTargetFormats;
        Format depthStencilFormat = Format::Unknown;
        u32 sampleCount = 1;

        StringView debugName;
    };

    // Alias for compatibility
    using GraphicsPipelineDesc = PipelineDesc;

    struct ComputePipelineDesc
    {
        IShader* computeShader = nullptr;
        StringView debugName;
    };

    class IPipeline
    {
    public:
        virtual ~IPipeline() = default;
    };

    // ========================================================================
    // Swapchain
    // ========================================================================

    struct SwapchainDesc
    {
        void* windowHandle;
        u32 width;
        u32 height;
        Format format = Format::R8G8B8A8_UNORM;
        u32 bufferCount = 2;
        bool vsync = true;
    };

    class ISwapchain
    {
    public:
        virtual ~ISwapchain() = default;

        [[nodiscard]] virtual u32 GetCurrentBackBufferIndex() const = 0;
        [[nodiscard]] virtual ITexture* GetBackBuffer(u32 index) = 0;
        [[nodiscard]] virtual ITexture* GetCurrentBackbuffer() = 0;
        [[nodiscard]] virtual u32 GetBackBufferCount() const = 0;
        [[nodiscard]] virtual Format GetFormat() const = 0;
        [[nodiscard]] virtual u32 GetWidth() const = 0;
        [[nodiscard]] virtual u32 GetHeight() const = 0;

        virtual void BeginFrame() = 0;
        virtual void Present() = 0;
        virtual void Resize(u32 width, u32 height) = 0;
        virtual void SetVSync(bool enabled) = 0;
    };

    // ========================================================================
    // Synchronization
    // ========================================================================

    class IFence
    {
    public:
        virtual ~IFence() = default;

        [[nodiscard]] virtual u64 GetCompletedValue() const = 0;
        virtual void Wait(u64 value) = 0;
        virtual void Signal(u64 value) = 0;
    };

    // ========================================================================
    // Command List
    // ========================================================================

    struct RenderPassDesc
    {
        struct ColorAttachment
        {
            ITexture* texture = nullptr;
            ClearValue clearValue;
            bool clear = true;
            bool store = true;
        };

        struct DepthAttachment
        {
            ITexture* texture = nullptr;
            f32 clearDepth = 1.0f;
            u8 clearStencil = 0;
            bool clearDepthValue = true;
            bool clearStencilValue = true;
            bool storeDepth = true;
            bool storeStencil = true;
        };

        Vector<ColorAttachment> colorAttachments;
        DepthAttachment depthAttachment;
    };

    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        // Lifecycle
        virtual void Begin() = 0;
        virtual void End() = 0;

        // Barriers
        virtual void ResourceBarrier(IBuffer* buffer, ResourceState before, ResourceState after) = 0;
        virtual void ResourceBarrier(ITexture* texture, ResourceState before, ResourceState after) = 0;

        // Render pass
        virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
        virtual void EndRenderPass() = 0;

        // Render targets (alternative to render pass)
        virtual void SetRenderTargets(Span<ITexture*> renderTargets, ITexture* depthStencil = nullptr) = 0;
        virtual void ClearRenderTarget(ITexture* texture, const Math::Vec4& color) = 0;
        virtual void ClearDepthStencil(ITexture* texture, f32 depth, u8 stencil) = 0;

        // Pipeline state
        virtual void SetPipeline(IPipeline* pipeline) = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor) = 0;

        // Buffers
        virtual void BindVertexBuffer(u32 slot, IBuffer* buffer, u32 stride, u32 offset = 0) = 0;
        virtual void BindIndexBuffer(IBuffer* buffer, IndexFormat format, u32 offset = 0) = 0;
        virtual void BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset = 0) = 0;

        // Textures
        virtual void BindTexture(u32 slot, ITexture* texture) = 0;

        // Legacy aliases
        void SetVertexBuffer(u32 slot, IBuffer* buffer, u32 stride, u32 offset = 0)
        { BindVertexBuffer(slot, buffer, stride, offset); }
        void SetIndexBuffer(IBuffer* buffer, IndexFormat format, u32 offset = 0)
        { BindIndexBuffer(buffer, format, offset); }
        void SetConstantBuffer(u32 slot, IBuffer* buffer, u32 offset = 0)
        { BindConstantBuffer(slot, buffer, offset); }

        // Draw commands
        virtual void Draw(u32 vertexCount, u32 firstVertex = 0) = 0;
        virtual void DrawIndexed(u32 indexCount, u32 firstIndex = 0, i32 vertexOffset = 0) = 0;
        virtual void DrawInstanced(u32 vertexCount, u32 instanceCount,
                                   u32 firstVertex = 0, u32 firstInstance = 0) = 0;
        virtual void DrawIndexedInstanced(u32 indexCount, u32 instanceCount,
                                          u32 firstIndex = 0, i32 vertexOffset = 0,
                                          u32 firstInstance = 0) = 0;

        // Compute
        virtual void Dispatch(u32 x, u32 y, u32 z) = 0;

        // Copy
        virtual void CopyBuffer(IBuffer* src, IBuffer* dst, usize srcOffset,
                               usize dstOffset, usize size) = 0;
        virtual void CopyTexture(ITexture* src, ITexture* dst) = 0;
        virtual void CopyBufferToTexture(IBuffer* src, ITexture* dst, u32 mipLevel = 0) = 0;

        // Debug
        virtual void BeginDebugMarker(StringView name) = 0;
        virtual void EndDebugMarker() = 0;

        // Native handle access (for ImGui integration, etc.)
        [[nodiscard]] virtual void* GetNativeCommandList() const = 0;
    };

    // ========================================================================
    // Parallel Command List Support
    // ========================================================================

    /// Configuration for command list pool
    struct CommandListPoolDesc
    {
        CommandListType type = CommandListType::Graphics;
        u32 initialPoolSize = 16;         ///< Initial command lists to pre-allocate
        u32 maxPoolSize = 64;             ///< Maximum command lists in the pool
        StringView debugName;
    };

    /// Thread-safe command list pool
    /// Allows parallel command list recording from multiple threads
    class ICommandListPool
    {
    public:
        virtual ~ICommandListPool() = default;

        /// Acquire a command list for recording (thread-safe)
        /// Returns nullptr if pool is exhausted and at max capacity
        [[nodiscard]] virtual ICommandList* Acquire() = 0;

        /// Release a command list back to the pool (thread-safe)
        /// Should be called after the command list has been submitted
        virtual void Release(ICommandList* cmdList) = 0;

        /// Reset all command lists in the pool (call at frame start)
        /// NOT thread-safe - call before any parallel recording
        virtual void Reset() = 0;

        /// Get the type of command lists in this pool
        [[nodiscard]] virtual CommandListType GetType() const = 0;

        /// Get number of command lists currently acquired
        [[nodiscard]] virtual u32 GetAcquiredCount() const = 0;

        /// Get total command lists in pool (free + acquired)
        [[nodiscard]] virtual u32 GetTotalCount() const = 0;
    };

    /// Context for parallel command list recording
    /// Manages command lists for a single parallel recording session
    struct ParallelCommandContext
    {
        ICommandListPool* pool = nullptr;
        Vector<ICommandList*> commandLists;  ///< Command lists recorded in this context
        u32 commandListCount = 0;

        /// Get a command list for the specified index
        /// Creates new command list if needed
        ICommandList* GetCommandList(u32 index)
        {
            while (commandLists.size() <= index)
            {
                commandLists.push_back(nullptr);
            }
            if (!commandLists[index])
            {
                commandLists[index] = pool->Acquire();
                if (commandLists[index])
                {
                    commandLists[index]->Begin();
                    commandListCount++;
                }
            }
            return commandLists[index];
        }

        /// End all command lists and prepare for submission
        void Finalize()
        {
            for (auto* cmdList : commandLists)
            {
                if (cmdList)
                {
                    cmdList->End();
                }
            }
        }

        /// Release all command lists back to pool
        void Release()
        {
            for (auto* cmdList : commandLists)
            {
                if (cmdList)
                {
                    pool->Release(cmdList);
                }
            }
            commandLists.clear();
            commandListCount = 0;
        }
    };

    // ========================================================================
    // Device
    // ========================================================================

    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        // Factory methods
        [[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;
        [[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& desc) = 0;
        [[nodiscard]] virtual UniquePtr<IShader> CreateShader(const ShaderDesc& desc) = 0;
        [[nodiscard]] virtual UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& desc) = 0;
        [[nodiscard]] virtual UniquePtr<IPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
        { return CreatePipeline(desc); }
        [[nodiscard]] virtual UniquePtr<IPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
        [[nodiscard]] virtual UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& desc) = 0;
        [[nodiscard]] virtual UniquePtr<ICommandList> CreateCommandList(CommandListType type = CommandListType::Graphics) = 0;
        [[nodiscard]] virtual UniquePtr<IFence> CreateFence(u64 initialValue = 0) = 0;
        [[nodiscard]] virtual UniquePtr<ICommandListPool> CreateCommandListPool(const CommandListPoolDesc& desc = {}) = 0;

        // Command execution
        virtual void Submit(ICommandList* commandList) = 0;
        virtual void Submit(Span<ICommandList*> commandLists) = 0;

        /// Submit a parallel command context (submits all recorded command lists in order)
        virtual void Submit(ParallelCommandContext& context)
        {
            context.Finalize();
            if (!context.commandLists.empty())
            {
                Vector<ICommandList*> validLists;
                validLists.reserve(context.commandLists.size());
                for (auto* cmdList : context.commandLists)
                {
                    if (cmdList)
                    {
                        validLists.push_back(cmdList);
                    }
                }
                if (!validLists.empty())
                {
                    Submit(Span<ICommandList*>(validLists.data(), validLists.size()));
                }
            }
            context.Release();
        }

        // Synchronization
        virtual void WaitIdle() = 0;
        virtual void SignalFence(IFence* fence, u64 value) = 0;
        virtual void WaitFence(IFence* fence, u64 value) = 0;

        // Frame management
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        [[nodiscard]] virtual u32 GetCurrentFrameIndex() const = 0;
        [[nodiscard]] virtual u32 GetFrameCount() const = 0;

        // Capabilities
        [[nodiscard]] virtual const DeviceCapabilities& GetCapabilities() const = 0;
        [[nodiscard]] virtual GraphicsAPI GetBackend() const = 0;

        // Native handle access (for ImGui integration, etc.)
        [[nodiscard]] virtual void* GetNativeDevice() const = 0;
        [[nodiscard]] virtual void* GetNativeSRVHeap() const = 0;

        // Execute a command list (alias for Submit)
        virtual void ExecuteCommandList(ICommandList* commandList) { Submit(commandList); }

        // Static factory
        static UniquePtr<IDevice> Create(const DeviceConfig& config = {});
    };

    // ========================================================================
    // Device Factory Function
    // ========================================================================

    /// Create a graphics device
    [[nodiscard]] UniquePtr<IDevice> CreateDevice(const DeviceConfig& config = {});

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /// Get bytes per pixel for a format
    [[nodiscard]] u32 GetFormatBytesPerPixel(Format format);

    /// Check if format is a depth format
    [[nodiscard]] bool IsDepthFormat(Format format);

    /// Check if format is a compressed format
    [[nodiscard]] bool IsCompressedFormat(Format format);

} // namespace Forge::RHI
