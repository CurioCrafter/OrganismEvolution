#include "Shaders/ShaderCompiler.h"
#include <fstream>
#include <sstream>
#include <filesystem>

// DXC headers (using the DX12 dxcapi.h)
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wrl/client.h>
#include <dxcapi.h>
#include <d3d12shader.h>
using Microsoft::WRL::ComPtr;
#endif

namespace Forge::Shaders
{
    // ========================================================================
    // Shader Compiler Implementation
    // ========================================================================

#ifdef _WIN32
    struct ShaderCompiler::Impl
    {
        ComPtr<IDxcUtils> utils;
        ComPtr<IDxcCompiler3> compiler;
        ComPtr<IDxcIncludeHandler> defaultIncludeHandler;

        bool Initialize()
        {
            HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
            if (FAILED(hr))
            {
                return false;
            }

            hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
            if (FAILED(hr))
            {
                return false;
            }

            hr = utils->CreateDefaultIncludeHandler(&defaultIncludeHandler);
            if (FAILED(hr))
            {
                return false;
            }

            return true;
        }

        void Shutdown()
        {
            defaultIncludeHandler.Reset();
            compiler.Reset();
            utils.Reset();
        }
    };
#else
    struct ShaderCompiler::Impl
    {
        bool Initialize() { return false; }
        void Shutdown() {}
    };
#endif

    ShaderCompiler::ShaderCompiler()
        : m_impl(MakeUnique<Impl>())
    {
    }

    ShaderCompiler::~ShaderCompiler()
    {
        Shutdown();
    }

    bool ShaderCompiler::Initialize()
    {
        if (!m_impl->Initialize())
        {
            m_lastError = "Failed to initialize DXC compiler";
            return false;
        }
        return true;
    }

    void ShaderCompiler::Shutdown()
    {
        m_impl->Shutdown();
    }

    ShaderCompileResult ShaderCompiler::CompileFromSource(
        StringView source,
        const ShaderCompileOptions& options)
    {
        ShaderCompileResult result;

#ifdef _WIN32
        if (!m_impl->compiler)
        {
            result.errorMessage = "Shader compiler not initialized";
            return result;
        }

        // Convert source to wide string for DXC
        std::wstring wideSource(source.begin(), source.end());

        // Create source blob
        ComPtr<IDxcBlobEncoding> sourceBlob;
        HRESULT hr = m_impl->utils->CreateBlob(
            source.data(),
            static_cast<UINT32>(source.size()),
            CP_UTF8,
            &sourceBlob);

        if (FAILED(hr))
        {
            result.errorMessage = "Failed to create source blob";
            return result;
        }

        // Build arguments
        std::vector<LPCWSTR> arguments;

        // Entry point
        std::wstring entryPointW(options.entryPoint.begin(), options.entryPoint.end());
        arguments.push_back(L"-E");
        arguments.push_back(entryPointW.c_str());

        // Shader model
        ShaderModel model = options.shaderModel;
        if (static_cast<u8>(model) == 0 && m_defaultShaderModel != ShaderModel::SM_6_0)
        {
            model = m_defaultShaderModel;
        }

        std::wstring targetProfile = [&]() -> std::wstring {
            const char* profileStr = GetShaderModelString(model, options.type);
            return std::wstring(profileStr, profileStr + strlen(profileStr));
        }();

        arguments.push_back(L"-T");
        arguments.push_back(targetProfile.c_str());

        // Optimization flags
        if ((options.flags & ShaderCompileFlags::SkipOptimization) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-Od");
        }
        else if ((options.flags & ShaderCompileFlags::OptimizationLevel3) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-O3");
        }
        else if ((options.flags & ShaderCompileFlags::OptimizationLevel2) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-O2");
        }
        else if ((options.flags & ShaderCompileFlags::OptimizationLevel1) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-O1");
        }
        else if ((options.flags & ShaderCompileFlags::OptimizationLevel0) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-O0");
        }

        // Debug info
        if ((options.flags & ShaderCompileFlags::Debug) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Qembed_debug");
        }

        // Warnings as errors
        if ((options.flags & ShaderCompileFlags::WarningsAsErrors) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-WX");
        }

        // Matrix packing
        if ((options.flags & ShaderCompileFlags::PackMatrixRowMajor) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-Zpr");
        }
        else if ((options.flags & ShaderCompileFlags::PackMatrixColumnMajor) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-Zpc");
        }

        // All resources bound
        if ((options.flags & ShaderCompileFlags::AllResourcesBound) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-all_resources_bound");
        }

        // Add defines
        std::vector<std::wstring> defineStrs;
        defineStrs.reserve(options.defines.size());
        for (const auto& define : options.defines)
        {
            std::wstring defineStr(define.name.begin(), define.name.end());
            defineStr += L"=";
            defineStr += std::wstring(define.value.begin(), define.value.end());
            defineStrs.push_back(std::move(defineStr));
        }
        for (const auto& defineStr : defineStrs)
        {
            arguments.push_back(L"-D");
            arguments.push_back(defineStr.c_str());
        }

        // Add include paths
        std::vector<std::wstring> includePathsW;
        for (const auto& path : m_globalIncludePaths)
        {
            includePathsW.emplace_back(path.begin(), path.end());
        }
        for (const auto& path : options.includePaths)
        {
            includePathsW.emplace_back(path.begin(), path.end());
        }
        for (const auto& path : includePathsW)
        {
            arguments.push_back(L"-I");
            arguments.push_back(path.c_str());
        }

        // Compile
        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = CP_UTF8;

        ComPtr<IDxcResult> compileResult;
        hr = m_impl->compiler->Compile(
            &sourceBuffer,
            arguments.data(),
            static_cast<UINT32>(arguments.size()),
            m_impl->defaultIncludeHandler.Get(),
            IID_PPV_ARGS(&compileResult));

        if (FAILED(hr))
        {
            result.errorMessage = "DXC Compile call failed";
            return result;
        }

        // Check compilation status
        HRESULT compileStatus;
        compileResult->GetStatus(&compileStatus);

        // Get errors/warnings
        ComPtr<IDxcBlobUtf8> errors;
        compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0)
        {
            std::string errStr(errors->GetStringPointer(), errors->GetStringLength());
            if (FAILED(compileStatus))
            {
                result.errorMessage = errStr;
            }
            else
            {
                result.warningMessage = errStr;
            }
        }

        if (FAILED(compileStatus))
        {
            return result;
        }

        // Get compiled bytecode
        ComPtr<IDxcBlob> shaderBlob;
        compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        if (shaderBlob && shaderBlob->GetBufferSize() > 0)
        {
            result.bytecode.resize(shaderBlob->GetBufferSize());
            memcpy(result.bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
            result.success = true;
        }

        // Get reflection data
        ComPtr<IDxcBlob> reflectionBlob;
        compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);
        if (reflectionBlob)
        {
            DxcBuffer reflectionBuffer;
            reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
            reflectionBuffer.Size = reflectionBlob->GetBufferSize();
            reflectionBuffer.Encoding = 0;

            ComPtr<ID3D12ShaderReflection> reflection;
            hr = m_impl->utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));
            if (SUCCEEDED(hr) && reflection)
            {
                D3D12_SHADER_DESC shaderDesc;
                reflection->GetDesc(&shaderDesc);

                // Extract constant buffer info
                for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
                {
                    ID3D12ShaderReflectionConstantBuffer* cbReflection = reflection->GetConstantBufferByIndex(i);
                    D3D12_SHADER_BUFFER_DESC cbDesc;
                    cbReflection->GetDesc(&cbDesc);

                    ShaderCompileResult::ConstantBufferLayout cb;
                    cb.name = cbDesc.Name;
                    cb.size = cbDesc.Size;

                    // Get binding info
                    D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                    for (UINT j = 0; j < shaderDesc.BoundResources; ++j)
                    {
                        reflection->GetResourceBindingDesc(j, &bindDesc);
                        if (strcmp(bindDesc.Name, cbDesc.Name) == 0)
                        {
                            cb.bindPoint = bindDesc.BindPoint;
                            cb.bindSpace = bindDesc.Space;
                            break;
                        }
                    }

                    // Get variables
                    for (UINT v = 0; v < cbDesc.Variables; ++v)
                    {
                        ID3D12ShaderReflectionVariable* varReflection = cbReflection->GetVariableByIndex(v);
                        D3D12_SHADER_VARIABLE_DESC varDesc;
                        varReflection->GetDesc(&varDesc);

                        ShaderCompileResult::ConstantBufferLayout::Variable var;
                        var.name = varDesc.Name;
                        var.offset = varDesc.StartOffset;
                        var.size = varDesc.Size;
                        cb.variables.push_back(std::move(var));
                    }

                    result.constantBuffers.push_back(std::move(cb));
                }

                // Extract resource bindings
                for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
                {
                    D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                    reflection->GetResourceBindingDesc(i, &bindDesc);

                    ShaderCompileResult::ResourceBinding binding;
                    binding.name = bindDesc.Name;
                    binding.bindPoint = bindDesc.BindPoint;
                    binding.bindSpace = bindDesc.Space;
                    binding.bindCount = bindDesc.BindCount;

                    switch (bindDesc.Type)
                    {
                    case D3D_SIT_CBUFFER:
                        binding.type = ShaderCompileResult::ResourceBinding::Type::ConstantBuffer;
                        break;
                    case D3D_SIT_TEXTURE:
                    case D3D_SIT_TBUFFER:
                        binding.type = ShaderCompileResult::ResourceBinding::Type::Texture;
                        break;
                    case D3D_SIT_SAMPLER:
                        binding.type = ShaderCompileResult::ResourceBinding::Type::Sampler;
                        break;
                    case D3D_SIT_UAV_RWTYPED:
                    case D3D_SIT_UAV_RWSTRUCTURED:
                    case D3D_SIT_UAV_RWBYTEADDRESS:
                    case D3D_SIT_UAV_APPEND_STRUCTURED:
                    case D3D_SIT_UAV_CONSUME_STRUCTURED:
                    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                        binding.type = ShaderCompileResult::ResourceBinding::Type::UAV;
                        break;
                    case D3D_SIT_STRUCTURED:
                    case D3D_SIT_BYTEADDRESS:
                        binding.type = ShaderCompileResult::ResourceBinding::Type::StructuredBuffer;
                        break;
                    default:
                        binding.type = ShaderCompileResult::ResourceBinding::Type::Texture;
                        break;
                    }

                    result.resourceBindings.push_back(std::move(binding));
                }
            }
        }
#else
        result.errorMessage = "Shader compilation not supported on this platform";
#endif

        return result;
    }

    ShaderCompileResult ShaderCompiler::CompileFromFile(
        StringView filePath,
        const ShaderCompileOptions& options)
    {
        ShaderCompileResult result;

        // Read file contents
        std::ifstream file(String(filePath), std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            result.errorMessage = "Failed to open shader file: " + String(filePath);
            return result;
        }

        usize fileSize = static_cast<usize>(file.tellg());
        file.seekg(0);

        String source;
        source.resize(fileSize);
        file.read(source.data(), fileSize);
        file.close();

        // Add file directory to include paths
        ShaderCompileOptions modifiedOptions = options;
        String pathStr(filePath);
        std::filesystem::path fsPath{pathStr};
        if (fsPath.has_parent_path())
        {
            modifiedOptions.includePaths.push_back(fsPath.parent_path().string());
        }

        return CompileFromSource(source, modifiedOptions);
    }

    void ShaderCompiler::AddIncludePath(StringView path)
    {
        m_globalIncludePaths.push_back(String(path));
    }

    void ShaderCompiler::SetDefaultShaderModel(ShaderModel model)
    {
        m_defaultShaderModel = model;
    }

    // ========================================================================
    // Shader Cache Implementation
    // ========================================================================

    usize ShaderCacheKeyHash::operator()(const ShaderCacheKey& key) const
    {
        usize h = std::hash<String>{}(key.sourcePath);
        h ^= std::hash<String>{}(key.entryPoint) << 1;
        h ^= std::hash<u8>{}(static_cast<u8>(key.type)) << 2;
        h ^= std::hash<u8>{}(static_cast<u8>(key.shaderModel)) << 3;
        h ^= std::hash<u64>{}(key.definesHash) << 4;
        return h;
    }

    bool ShaderCache::Initialize(StringView cacheDirectory)
    {
        m_cacheDirectory = String(cacheDirectory);

        if (!m_cacheDirectory.empty())
        {
            std::filesystem::create_directories(m_cacheDirectory);
        }

        return true;
    }

    bool ShaderCache::HasValidCache(const ShaderCacheKey& key, u64 sourceTimestamp) const
    {
        auto it = m_memoryCache.find(key);
        if (it != m_memoryCache.end())
        {
            if (it->second.sourceTimestamp >= sourceTimestamp)
            {
                ++m_cacheHits;
                return true;
            }
        }
        ++m_cacheMisses;
        return false;
    }

    Vector<u8> ShaderCache::LoadCached(const ShaderCacheKey& key) const
    {
        auto it = m_memoryCache.find(key);
        if (it != m_memoryCache.end())
        {
            return it->second.bytecode;
        }
        return {};
    }

    void ShaderCache::Store(const ShaderCacheKey& key, Span<const u8> bytecode, u64 sourceTimestamp)
    {
        CacheEntry entry;
        entry.bytecode.assign(bytecode.begin(), bytecode.end());
        entry.sourceTimestamp = sourceTimestamp;
        m_memoryCache[key] = std::move(entry);
    }

    void ShaderCache::ClearCache()
    {
        m_memoryCache.clear();
        m_cacheHits = 0;
        m_cacheMisses = 0;
    }

    // ========================================================================
    // Shader Library Implementation
    // ========================================================================

    bool ShaderLibrary::Initialize(RHI::IDevice* device, StringView cacheDirectory)
    {
        m_device = device;

        if (!m_compiler.Initialize())
        {
            return false;
        }

        return m_cache.Initialize(cacheDirectory);
    }

    void ShaderLibrary::Shutdown()
    {
        m_shaders.clear();
        m_cache.ClearCache();
        m_compiler.Shutdown();
        m_device = nullptr;
    }

    RHI::IShader* ShaderLibrary::GetShader(
        StringView path,
        const ShaderCompileOptions& options)
    {
        String pathStr(path);

        // Check if already loaded
        auto it = m_shaders.find(pathStr);
        if (it != m_shaders.end())
        {
            // TODO: Check if source file changed (hot reload)
            return it->second.shader.get();
        }

        // Create cache key
        ShaderCacheKey cacheKey;
        cacheKey.sourcePath = pathStr;
        cacheKey.entryPoint = String(options.entryPoint);
        cacheKey.type = options.type;
        cacheKey.shaderModel = options.shaderModel;
        cacheKey.definesHash = CalculateDefinesHash(options.defines);

        // Get file timestamp
        u64 fileTimestamp = 0;
        if (std::filesystem::exists(pathStr))
        {
            auto ftime = std::filesystem::last_write_time(pathStr);
            fileTimestamp = static_cast<u64>(ftime.time_since_epoch().count());
        }

        // Check cache
        Vector<u8> bytecode;
        if (m_cache.HasValidCache(cacheKey, fileTimestamp))
        {
            bytecode = m_cache.LoadCached(cacheKey);
            m_stats.cacheHits++;
        }
        else
        {
            // Compile shader
            auto result = m_compiler.CompileFromFile(path, options);
            if (!result.success)
            {
                m_stats.compilationErrors++;
                return nullptr;
            }

            bytecode = std::move(result.bytecode);
            m_cache.Store(cacheKey, bytecode, fileTimestamp);
            m_stats.cacheMisses++;
            m_stats.totalCompilations++;
        }

        // Create RHI shader
        RHI::ShaderDesc desc;
        desc.type = options.type;
        desc.bytecode = Span<const u8>(bytecode.data(), bytecode.size());
        desc.entryPoint = options.entryPoint;
        desc.debugName = path;

        auto shader = m_device->CreateShader(desc);
        if (!shader)
        {
            return nullptr;
        }

        ShaderEntry entry;
        entry.shader = std::move(shader);
        entry.cacheKey = cacheKey;
        entry.lastModified = fileTimestamp;

        RHI::IShader* result = entry.shader.get();
        m_shaders[pathStr] = std::move(entry);

        return result;
    }

    RHI::IShader* ShaderLibrary::RecompileShader(
        StringView path,
        const ShaderCompileOptions& options)
    {
        // Remove from cache
        String pathStr(path);
        m_shaders.erase(pathStr);

        // Recompile
        return GetShader(path, options);
    }

    void ShaderLibrary::AddIncludePath(StringView path)
    {
        m_compiler.AddIncludePath(path);
    }

    void ShaderLibrary::ClearShaders()
    {
        m_shaders.clear();
    }

    ShaderLibrary::Stats ShaderLibrary::GetStats() const
    {
        Stats stats = m_stats;
        stats.cacheHits = m_cache.GetCacheHits();
        stats.cacheMisses = m_cache.GetCacheMisses();
        return stats;
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    RHI::ShaderType GetShaderTypeFromExtension(StringView path)
    {
        String pathStr(path);
        std::filesystem::path fsPath(pathStr);
        auto ext = fsPath.extension().string();

        // Common naming conventions
        auto stem = fsPath.stem().string();
        if (stem.ends_with(".vs") || stem.ends_with("_vs") || ext == ".vert")
            return RHI::ShaderType::Vertex;
        if (stem.ends_with(".ps") || stem.ends_with("_ps") || ext == ".frag" || ext == ".pixel")
            return RHI::ShaderType::Pixel;
        if (stem.ends_with(".gs") || stem.ends_with("_gs") || ext == ".geom")
            return RHI::ShaderType::Geometry;
        if (stem.ends_with(".hs") || stem.ends_with("_hs") || ext == ".hull")
            return RHI::ShaderType::Hull;
        if (stem.ends_with(".ds") || stem.ends_with("_ds") || ext == ".domain")
            return RHI::ShaderType::Domain;
        if (stem.ends_with(".cs") || stem.ends_with("_cs") || ext == ".comp")
            return RHI::ShaderType::Compute;
        if (stem.ends_with(".ms") || stem.ends_with("_ms") || ext == ".mesh")
            return RHI::ShaderType::Mesh;
        if (stem.ends_with(".as") || stem.ends_with("_as") || ext == ".task")
            return RHI::ShaderType::Amplification;

        // Default to vertex
        return RHI::ShaderType::Vertex;
    }

    StringView GetDefaultEntryPoint(RHI::ShaderType type)
    {
        switch (type)
        {
        case RHI::ShaderType::Vertex: return "VSMain";
        case RHI::ShaderType::Pixel: return "PSMain";
        case RHI::ShaderType::Compute: return "CSMain";
        case RHI::ShaderType::Geometry: return "GSMain";
        case RHI::ShaderType::Hull: return "HSMain";
        case RHI::ShaderType::Domain: return "DSMain";
        case RHI::ShaderType::Mesh: return "MSMain";
        case RHI::ShaderType::Amplification: return "ASMain";
        default: return "main";
        }
    }

    const char* GetShaderModelString(ShaderModel model, RHI::ShaderType type)
    {
        const char* typeStr = "";
        switch (type)
        {
        case RHI::ShaderType::Vertex: typeStr = "vs"; break;
        case RHI::ShaderType::Pixel: typeStr = "ps"; break;
        case RHI::ShaderType::Compute: typeStr = "cs"; break;
        case RHI::ShaderType::Geometry: typeStr = "gs"; break;
        case RHI::ShaderType::Hull: typeStr = "hs"; break;
        case RHI::ShaderType::Domain: typeStr = "ds"; break;
        case RHI::ShaderType::Mesh: typeStr = "ms"; break;
        case RHI::ShaderType::Amplification: typeStr = "as"; break;
        }

        // Return static strings for each combination
        switch (model)
        {
        case ShaderModel::SM_6_0:
            switch (type)
            {
            case RHI::ShaderType::Vertex: return "vs_6_0";
            case RHI::ShaderType::Pixel: return "ps_6_0";
            case RHI::ShaderType::Compute: return "cs_6_0";
            case RHI::ShaderType::Geometry: return "gs_6_0";
            case RHI::ShaderType::Hull: return "hs_6_0";
            case RHI::ShaderType::Domain: return "ds_6_0";
            default: return "vs_6_0";
            }
        case ShaderModel::SM_6_1:
            switch (type)
            {
            case RHI::ShaderType::Vertex: return "vs_6_1";
            case RHI::ShaderType::Pixel: return "ps_6_1";
            case RHI::ShaderType::Compute: return "cs_6_1";
            case RHI::ShaderType::Geometry: return "gs_6_1";
            case RHI::ShaderType::Hull: return "hs_6_1";
            case RHI::ShaderType::Domain: return "ds_6_1";
            default: return "vs_6_1";
            }
        case ShaderModel::SM_6_2:
            switch (type)
            {
            case RHI::ShaderType::Vertex: return "vs_6_2";
            case RHI::ShaderType::Pixel: return "ps_6_2";
            case RHI::ShaderType::Compute: return "cs_6_2";
            case RHI::ShaderType::Geometry: return "gs_6_2";
            case RHI::ShaderType::Hull: return "hs_6_2";
            case RHI::ShaderType::Domain: return "ds_6_2";
            default: return "vs_6_2";
            }
        case ShaderModel::SM_6_5:
            switch (type)
            {
            case RHI::ShaderType::Vertex: return "vs_6_5";
            case RHI::ShaderType::Pixel: return "ps_6_5";
            case RHI::ShaderType::Compute: return "cs_6_5";
            case RHI::ShaderType::Geometry: return "gs_6_5";
            case RHI::ShaderType::Hull: return "hs_6_5";
            case RHI::ShaderType::Domain: return "ds_6_5";
            case RHI::ShaderType::Mesh: return "ms_6_5";
            case RHI::ShaderType::Amplification: return "as_6_5";
            default: return "vs_6_5";
            }
        case ShaderModel::SM_6_6:
            switch (type)
            {
            case RHI::ShaderType::Vertex: return "vs_6_6";
            case RHI::ShaderType::Pixel: return "ps_6_6";
            case RHI::ShaderType::Compute: return "cs_6_6";
            case RHI::ShaderType::Geometry: return "gs_6_6";
            case RHI::ShaderType::Hull: return "hs_6_6";
            case RHI::ShaderType::Domain: return "ds_6_6";
            case RHI::ShaderType::Mesh: return "ms_6_6";
            case RHI::ShaderType::Amplification: return "as_6_6";
            default: return "vs_6_6";
            }
        default:
            return "vs_6_0";
        }
    }

    u64 CalculateDefinesHash(Span<const ShaderMacro> defines)
    {
        u64 hash = 0;
        for (const auto& define : defines)
        {
            hash ^= std::hash<String>{}(define.name);
            hash ^= std::hash<String>{}(define.value) << 1;
        }
        return hash;
    }

} // namespace Forge::Shaders
