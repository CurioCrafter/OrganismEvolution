#pragma once

// DirectX 12 Shader System for OrganismEvolution
// Handles HLSL shader compilation, PSO management, root signatures, and hot-reload

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <thread>

using Microsoft::WRL::ComPtr;

namespace DX12
{
    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class ShaderCompiler;
    class Shader;
    class ShaderProgram;
    class PipelineStateManager;
    class FileWatcher;

    // ============================================================================
    // Enums and Constants
    // ============================================================================

    enum class ShaderType
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute,
        Mesh,
        Amplification
    };

    enum class ShaderModel
    {
        SM_6_0,
        SM_6_1,
        SM_6_2,
        SM_6_3,
        SM_6_4,
        SM_6_5,
        SM_6_6
    };

    enum class ShaderCompileFlags : uint32_t
    {
        None                  = 0,
        Debug                 = 1 << 0,
        SkipOptimization      = 1 << 1,
        OptimizationLevel0    = 1 << 2,
        OptimizationLevel1    = 1 << 3,
        OptimizationLevel2    = 1 << 4,
        OptimizationLevel3    = 1 << 5,
        WarningsAsErrors      = 1 << 6,
        PackMatrixRowMajor    = 1 << 7,
        PackMatrixColumnMajor = 1 << 8,
        AllResourcesBound     = 1 << 9
    };

    inline ShaderCompileFlags operator|(ShaderCompileFlags a, ShaderCompileFlags b)
    {
        return static_cast<ShaderCompileFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline ShaderCompileFlags operator&(ShaderCompileFlags a, ShaderCompileFlags b)
    {
        return static_cast<ShaderCompileFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool operator!=(ShaderCompileFlags a, ShaderCompileFlags b)
    {
        return static_cast<uint32_t>(a) != static_cast<uint32_t>(b);
    }

    // ============================================================================
    // Shader Macro Definition
    // ============================================================================

    struct ShaderMacro
    {
        std::string name;
        std::string value;

        ShaderMacro() = default;
        ShaderMacro(const std::string& n, const std::string& v) : name(n), value(v) {}
    };

    // ============================================================================
    // Shader Compilation Options
    // ============================================================================

    struct ShaderCompileOptions
    {
        std::string entryPoint = "main";
        ShaderType type = ShaderType::Vertex;
        ShaderModel shaderModel = ShaderModel::SM_6_0;
        ShaderCompileFlags flags = ShaderCompileFlags::None;
        std::vector<ShaderMacro> defines;
        std::vector<std::string> includePaths;
    };

    // ============================================================================
    // Shader Compilation Result
    // ============================================================================

    struct ShaderCompileResult
    {
        bool success = false;
        std::vector<uint8_t> bytecode;
        std::string errorMessage;
        std::string warningMessage;

        // Reflection data
        struct ConstantBufferLayout
        {
            std::string name;
            uint32_t size = 0;
            uint32_t bindPoint = 0;
            uint32_t bindSpace = 0;

            struct Variable
            {
                std::string name;
                uint32_t offset = 0;
                uint32_t size = 0;
            };
            std::vector<Variable> variables;
        };
        std::vector<ConstantBufferLayout> constantBuffers;

        struct ResourceBinding
        {
            enum class Type
            {
                ConstantBuffer,
                Texture,
                Sampler,
                UAV,
                StructuredBuffer
            };

            std::string name;
            Type type = Type::Texture;
            uint32_t bindPoint = 0;
            uint32_t bindSpace = 0;
            uint32_t bindCount = 1;
        };
        std::vector<ResourceBinding> resourceBindings;
    };

    // ============================================================================
    // Shader Compiler (DXC-based)
    // ============================================================================

    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        bool Initialize();
        void Shutdown();

        // Compile from source string
        ShaderCompileResult CompileFromSource(
            const std::string& source,
            const ShaderCompileOptions& options);

        // Compile from file
        ShaderCompileResult CompileFromFile(
            const std::string& filePath,
            const ShaderCompileOptions& options);

        // Global include paths
        void AddIncludePath(const std::string& path);
        void ClearIncludePaths();

        // Default shader model
        void SetDefaultShaderModel(ShaderModel model) { m_defaultShaderModel = model; }
        ShaderModel GetDefaultShaderModel() const { return m_defaultShaderModel; }

        // Error handling
        const std::string& GetLastError() const { return m_lastError; }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

        std::vector<std::string> m_globalIncludePaths;
        ShaderModel m_defaultShaderModel = ShaderModel::SM_6_0;
        std::string m_lastError;

        static const wchar_t* GetTargetProfile(ShaderType type, ShaderModel model);
    };

    // ============================================================================
    // Individual Shader (Compiled Bytecode)
    // ============================================================================

    class Shader
    {
    public:
        Shader() = default;
        Shader(ShaderType type, std::vector<uint8_t> bytecode, const std::string& name = "");

        bool IsValid() const { return !m_bytecode.empty(); }
        ShaderType GetType() const { return m_type; }
        const std::vector<uint8_t>& GetBytecode() const { return m_bytecode; }
        D3D12_SHADER_BYTECODE GetD3D12Bytecode() const;
        const std::string& GetName() const { return m_name; }

        // Reflection data
        const ShaderCompileResult::ConstantBufferLayout* GetConstantBuffer(const std::string& name) const;
        const std::vector<ShaderCompileResult::ConstantBufferLayout>& GetConstantBuffers() const { return m_constantBuffers; }
        const std::vector<ShaderCompileResult::ResourceBinding>& GetResourceBindings() const { return m_resourceBindings; }

        void SetReflectionData(
            std::vector<ShaderCompileResult::ConstantBufferLayout> cbLayouts,
            std::vector<ShaderCompileResult::ResourceBinding> bindings);

    private:
        ShaderType m_type = ShaderType::Vertex;
        std::vector<uint8_t> m_bytecode;
        std::string m_name;
        std::vector<ShaderCompileResult::ConstantBufferLayout> m_constantBuffers;
        std::vector<ShaderCompileResult::ResourceBinding> m_resourceBindings;
    };

    // ============================================================================
    // Root Signature Description Builder
    // ============================================================================

    class RootSignatureBuilder
    {
    public:
        RootSignatureBuilder() = default;

        // Add root parameters
        RootSignatureBuilder& AddConstantBufferView(uint32_t shaderRegister, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureBuilder& AddShaderResourceView(uint32_t shaderRegister, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureBuilder& AddUnorderedAccessView(uint32_t shaderRegister, uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureBuilder& AddRootConstants(uint32_t num32BitValues, uint32_t shaderRegister,
            uint32_t space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureBuilder& AddDescriptorTable(
            const std::vector<D3D12_DESCRIPTOR_RANGE1>& ranges,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

        // Add static samplers
        RootSignatureBuilder& AddStaticSampler(
            uint32_t shaderRegister,
            D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            uint32_t space = 0,
            D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL);

        // Build the root signature
        ComPtr<ID3D12RootSignature> Build(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // Reset for reuse
        void Reset();

    private:
        std::vector<D3D12_ROOT_PARAMETER1> m_parameters;
        std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplers;
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> m_descriptorRanges;
    };

    // ============================================================================
    // Pipeline State Object Description
    // ============================================================================

    struct PipelineStateDesc
    {
        // Shaders
        const Shader* vertexShader = nullptr;
        const Shader* pixelShader = nullptr;
        const Shader* geometryShader = nullptr;
        const Shader* hullShader = nullptr;
        const Shader* domainShader = nullptr;

        // Root signature
        ID3D12RootSignature* rootSignature = nullptr;

        // Input layout
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

        // Render target formats
        std::vector<DXGI_FORMAT> rtvFormats;
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

        // Rasterizer state
        D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
        D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
        bool frontCounterClockwise = false;
        int32_t depthBias = 0;
        float depthBiasClamp = 0.0f;
        float slopeScaledDepthBias = 0.0f;
        bool depthClipEnable = true;
        bool multisampleEnable = false;
        bool antialiasedLineEnable = false;

        // Blend state
        bool blendEnable = false;
        D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
        D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
        D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;
        D3D12_BLEND srcBlendAlpha = D3D12_BLEND_ONE;
        D3D12_BLEND destBlendAlpha = D3D12_BLEND_ZERO;
        D3D12_BLEND_OP blendOpAlpha = D3D12_BLEND_OP_ADD;
        uint8_t renderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // Depth stencil state
        bool depthEnable = true;
        bool depthWriteEnable = true;
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
        bool stencilEnable = false;

        // Primitive topology
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // Sample description
        uint32_t sampleCount = 1;
        uint32_t sampleQuality = 0;

        // Debug name
        std::string debugName;
    };

    // ============================================================================
    // Compute Pipeline State Description
    // ============================================================================

    struct ComputePipelineStateDesc
    {
        const Shader* computeShader = nullptr;
        ID3D12RootSignature* rootSignature = nullptr;
        std::string debugName;
    };

    // ============================================================================
    // Pipeline State Object Wrapper
    // ============================================================================

    class PipelineState
    {
    public:
        PipelineState() = default;
        PipelineState(ComPtr<ID3D12PipelineState> pso, const std::string& name = "");

        bool IsValid() const { return m_pso != nullptr; }
        ID3D12PipelineState* Get() const { return m_pso.Get(); }
        ID3D12PipelineState* operator->() const { return m_pso.Get(); }
        const std::string& GetName() const { return m_name; }

    private:
        ComPtr<ID3D12PipelineState> m_pso;
        std::string m_name;
    };

    // ============================================================================
    // Constant Buffer Helper
    // ============================================================================

    template<typename T>
    class ConstantBuffer
    {
    public:
        ConstantBuffer() = default;

        bool Initialize(ID3D12Device* device, uint32_t frameCount = 2);
        void Update(const T& data, uint32_t frameIndex);
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const;
        void Release();

    private:
        ComPtr<ID3D12Resource> m_buffer;
        uint8_t* m_mappedData = nullptr;
        uint32_t m_alignedSize = 0;
        uint32_t m_frameCount = 0;
    };

    // ============================================================================
    // File Watcher for Hot-Reload
    // ============================================================================

    class FileWatcher
    {
    public:
        using FileChangedCallback = std::function<void(const std::string& filePath)>;

        FileWatcher();
        ~FileWatcher();

        bool Start(const std::string& directory, FileChangedCallback callback);
        void Stop();
        bool IsRunning() const { return m_running; }

        // Add/remove specific files to watch
        void AddFile(const std::string& filePath);
        void RemoveFile(const std::string& filePath);
        void ClearFiles();

    private:
        void WatchThread();

        std::thread m_watchThread;
        std::atomic<bool> m_running{false};
        std::string m_directory;
        FileChangedCallback m_callback;

        std::mutex m_filesMutex;
        std::unordered_map<std::string, std::filesystem::file_time_type> m_watchedFiles;

        HANDLE m_stopEvent = nullptr;
    };

    // ============================================================================
    // Shader Program (Combined VS/PS/etc. with PSO)
    // ============================================================================

    class ShaderProgram
    {
    public:
        ShaderProgram() = default;
        ShaderProgram(const std::string& name);

        // Load shaders from file
        bool LoadVertexShader(ShaderCompiler& compiler, const std::string& path,
            const ShaderCompileOptions& options = {});
        bool LoadPixelShader(ShaderCompiler& compiler, const std::string& path,
            const ShaderCompileOptions& options = {});
        bool LoadGeometryShader(ShaderCompiler& compiler, const std::string& path,
            const ShaderCompileOptions& options = {});
        bool LoadHullShader(ShaderCompiler& compiler, const std::string& path,
            const ShaderCompileOptions& options = {});
        bool LoadDomainShader(ShaderCompiler& compiler, const std::string& path,
            const ShaderCompileOptions& options = {});
        bool LoadComputeShader(ShaderCompiler& compiler, const std::string& path,
            const ShaderCompileOptions& options = {});

        // Create PSO
        bool CreatePipelineState(ID3D12Device* device, const PipelineStateDesc& desc);
        bool CreateComputePipelineState(ID3D12Device* device, const ComputePipelineStateDesc& desc);

        // Set root signature
        void SetRootSignature(ComPtr<ID3D12RootSignature> rootSig) { m_rootSignature = rootSig; }

        // Getters
        const Shader* GetVertexShader() const { return m_vertexShader.IsValid() ? &m_vertexShader : nullptr; }
        const Shader* GetPixelShader() const { return m_pixelShader.IsValid() ? &m_pixelShader : nullptr; }
        const Shader* GetGeometryShader() const { return m_geometryShader.IsValid() ? &m_geometryShader : nullptr; }
        const Shader* GetComputeShader() const { return m_computeShader.IsValid() ? &m_computeShader : nullptr; }

        ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
        ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }

        bool IsValid() const { return m_pipelineState.IsValid(); }
        const std::string& GetName() const { return m_name; }
        const std::string& GetLastError() const { return m_lastError; }

        // Hot-reload support
        void MarkDirty() { m_dirty = true; }
        bool IsDirty() const { return m_dirty; }
        void ClearDirty() { m_dirty = false; }

        // File paths for reload
        const std::string& GetVertexShaderPath() const { return m_vsPath; }
        const std::string& GetPixelShaderPath() const { return m_psPath; }

    private:
        std::string m_name;
        std::string m_lastError;

        Shader m_vertexShader;
        Shader m_pixelShader;
        Shader m_geometryShader;
        Shader m_hullShader;
        Shader m_domainShader;
        Shader m_computeShader;

        PipelineState m_pipelineState;
        ComPtr<ID3D12RootSignature> m_rootSignature;

        // File paths for hot-reload
        std::string m_vsPath;
        std::string m_psPath;
        std::string m_gsPath;
        std::string m_hsPath;
        std::string m_dsPath;
        std::string m_csPath;

        std::atomic<bool> m_dirty{false};
    };

    // ============================================================================
    // Shader Manager (Caching and Hot-Reload)
    // ============================================================================

    class ShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager();

        bool Initialize(ID3D12Device* device, const std::string& shaderDirectory);
        void Shutdown();

        // Get or create shader program
        ShaderProgram* GetProgram(const std::string& name);
        ShaderProgram* CreateProgram(const std::string& name);

        // Load a complete shader program from HLSL file (assumes combined VS/PS)
        ShaderProgram* LoadProgram(
            const std::string& name,
            const std::string& hlslPath,
            const PipelineStateDesc& psoDesc,
            const std::string& vsEntry = "VSMain",
            const std::string& psEntry = "PSMain");

        // Hot-reload
        void EnableHotReload(bool enable);
        bool IsHotReloadEnabled() const { return m_hotReloadEnabled; }
        void CheckForReloads();
        void ReloadAllDirty();

        // Compiler access
        ShaderCompiler& GetCompiler() { return m_compiler; }

        // Device access
        ID3D12Device* GetDevice() const { return m_device; }

    private:
        void OnFileChanged(const std::string& filePath);

        ID3D12Device* m_device = nullptr;
        std::string m_shaderDirectory;
        ShaderCompiler m_compiler;

        std::mutex m_programsMutex;
        std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> m_programs;

        // Hot-reload
        bool m_hotReloadEnabled = false;
        std::unique_ptr<FileWatcher> m_fileWatcher;
        std::mutex m_dirtyFilesMutex;
        std::vector<std::string> m_dirtyFiles;
    };

    // ============================================================================
    // Template Implementation: ConstantBuffer
    // ============================================================================

    template<typename T>
    bool ConstantBuffer<T>::Initialize(ID3D12Device* device, uint32_t frameCount)
    {
        m_frameCount = frameCount;
        m_alignedSize = (sizeof(T) + 255) & ~255; // 256-byte alignment for constant buffers

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = static_cast<UINT64>(m_alignedSize) * frameCount;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_buffer));

        if (FAILED(hr))
        {
            return false;
        }

        // Map the buffer persistently
        D3D12_RANGE readRange = {0, 0};
        hr = m_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData));
        if (FAILED(hr))
        {
            m_buffer.Reset();
            return false;
        }

        return true;
    }

    template<typename T>
    void ConstantBuffer<T>::Update(const T& data, uint32_t frameIndex)
    {
        if (m_mappedData && frameIndex < m_frameCount)
        {
            memcpy(m_mappedData + (frameIndex * m_alignedSize), &data, sizeof(T));
        }
    }

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer<T>::GetGPUVirtualAddress(uint32_t frameIndex) const
    {
        if (m_buffer && frameIndex < m_frameCount)
        {
            return m_buffer->GetGPUVirtualAddress() + (frameIndex * m_alignedSize);
        }
        return 0;
    }

    template<typename T>
    void ConstantBuffer<T>::Release()
    {
        if (m_buffer && m_mappedData)
        {
            m_buffer->Unmap(0, nullptr);
            m_mappedData = nullptr;
        }
        m_buffer.Reset();
    }

} // namespace DX12

#endif // _WIN32
