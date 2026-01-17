// DirectX 12 Shader System Implementation for OrganismEvolution
// Implements HLSL compilation via DXC, PSO management, root signatures, and hot-reload

#include "Shader_DX12.h"

#ifdef _WIN32

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>

// Link required libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

namespace DX12
{
    // ============================================================================
    // ShaderCompiler Implementation
    // ============================================================================

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

    ShaderCompiler::ShaderCompiler()
        : m_impl(std::make_unique<Impl>())
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
            m_lastError = "Failed to initialize DXC compiler. Ensure dxcompiler.dll is available.";
            return false;
        }
        return true;
    }

    void ShaderCompiler::Shutdown()
    {
        m_impl->Shutdown();
    }

    const wchar_t* ShaderCompiler::GetTargetProfile(ShaderType type, ShaderModel model)
    {
        // Build target profile string based on shader type and model
        struct ProfileEntry
        {
            ShaderType type;
            ShaderModel model;
            const wchar_t* profile;
        };

        static const ProfileEntry profiles[] = {
            // SM 6.0
            {ShaderType::Vertex, ShaderModel::SM_6_0, L"vs_6_0"},
            {ShaderType::Pixel, ShaderModel::SM_6_0, L"ps_6_0"},
            {ShaderType::Geometry, ShaderModel::SM_6_0, L"gs_6_0"},
            {ShaderType::Hull, ShaderModel::SM_6_0, L"hs_6_0"},
            {ShaderType::Domain, ShaderModel::SM_6_0, L"ds_6_0"},
            {ShaderType::Compute, ShaderModel::SM_6_0, L"cs_6_0"},

            // SM 6.1
            {ShaderType::Vertex, ShaderModel::SM_6_1, L"vs_6_1"},
            {ShaderType::Pixel, ShaderModel::SM_6_1, L"ps_6_1"},
            {ShaderType::Geometry, ShaderModel::SM_6_1, L"gs_6_1"},
            {ShaderType::Hull, ShaderModel::SM_6_1, L"hs_6_1"},
            {ShaderType::Domain, ShaderModel::SM_6_1, L"ds_6_1"},
            {ShaderType::Compute, ShaderModel::SM_6_1, L"cs_6_1"},

            // SM 6.2
            {ShaderType::Vertex, ShaderModel::SM_6_2, L"vs_6_2"},
            {ShaderType::Pixel, ShaderModel::SM_6_2, L"ps_6_2"},
            {ShaderType::Geometry, ShaderModel::SM_6_2, L"gs_6_2"},
            {ShaderType::Hull, ShaderModel::SM_6_2, L"hs_6_2"},
            {ShaderType::Domain, ShaderModel::SM_6_2, L"ds_6_2"},
            {ShaderType::Compute, ShaderModel::SM_6_2, L"cs_6_2"},

            // SM 6.3
            {ShaderType::Vertex, ShaderModel::SM_6_3, L"vs_6_3"},
            {ShaderType::Pixel, ShaderModel::SM_6_3, L"ps_6_3"},
            {ShaderType::Geometry, ShaderModel::SM_6_3, L"gs_6_3"},
            {ShaderType::Hull, ShaderModel::SM_6_3, L"hs_6_3"},
            {ShaderType::Domain, ShaderModel::SM_6_3, L"ds_6_3"},
            {ShaderType::Compute, ShaderModel::SM_6_3, L"cs_6_3"},

            // SM 6.4
            {ShaderType::Vertex, ShaderModel::SM_6_4, L"vs_6_4"},
            {ShaderType::Pixel, ShaderModel::SM_6_4, L"ps_6_4"},
            {ShaderType::Geometry, ShaderModel::SM_6_4, L"gs_6_4"},
            {ShaderType::Hull, ShaderModel::SM_6_4, L"hs_6_4"},
            {ShaderType::Domain, ShaderModel::SM_6_4, L"ds_6_4"},
            {ShaderType::Compute, ShaderModel::SM_6_4, L"cs_6_4"},

            // SM 6.5 (includes mesh shaders)
            {ShaderType::Vertex, ShaderModel::SM_6_5, L"vs_6_5"},
            {ShaderType::Pixel, ShaderModel::SM_6_5, L"ps_6_5"},
            {ShaderType::Geometry, ShaderModel::SM_6_5, L"gs_6_5"},
            {ShaderType::Hull, ShaderModel::SM_6_5, L"hs_6_5"},
            {ShaderType::Domain, ShaderModel::SM_6_5, L"ds_6_5"},
            {ShaderType::Compute, ShaderModel::SM_6_5, L"cs_6_5"},
            {ShaderType::Mesh, ShaderModel::SM_6_5, L"ms_6_5"},
            {ShaderType::Amplification, ShaderModel::SM_6_5, L"as_6_5"},

            // SM 6.6
            {ShaderType::Vertex, ShaderModel::SM_6_6, L"vs_6_6"},
            {ShaderType::Pixel, ShaderModel::SM_6_6, L"ps_6_6"},
            {ShaderType::Geometry, ShaderModel::SM_6_6, L"gs_6_6"},
            {ShaderType::Hull, ShaderModel::SM_6_6, L"hs_6_6"},
            {ShaderType::Domain, ShaderModel::SM_6_6, L"ds_6_6"},
            {ShaderType::Compute, ShaderModel::SM_6_6, L"cs_6_6"},
            {ShaderType::Mesh, ShaderModel::SM_6_6, L"ms_6_6"},
            {ShaderType::Amplification, ShaderModel::SM_6_6, L"as_6_6"},
        };

        for (const auto& entry : profiles)
        {
            if (entry.type == type && entry.model == model)
            {
                return entry.profile;
            }
        }

        // Default fallback
        return L"vs_6_0";
    }

    ShaderCompileResult ShaderCompiler::CompileFromSource(
        const std::string& source,
        const ShaderCompileOptions& options)
    {
        ShaderCompileResult result;

        if (!m_impl->compiler)
        {
            result.errorMessage = "Shader compiler not initialized";
            return result;
        }

        // Create source blob from UTF-8 string
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

        // Build compiler arguments
        std::vector<LPCWSTR> arguments;
        std::vector<std::wstring> argumentStorage; // Keep strings alive

        // Entry point
        std::wstring entryPointW(options.entryPoint.begin(), options.entryPoint.end());
        arguments.push_back(L"-E");
        arguments.push_back(entryPointW.c_str());

        // Target profile
        ShaderModel model = options.shaderModel;
        if (model == ShaderModel::SM_6_0 && m_defaultShaderModel != ShaderModel::SM_6_0)
        {
            model = m_defaultShaderModel;
        }

        const wchar_t* targetProfile = GetTargetProfile(options.type, model);
        arguments.push_back(L"-T");
        arguments.push_back(targetProfile);

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
            arguments.push_back(L"-Zi");       // Debug info
            arguments.push_back(L"-Qembed_debug"); // Embed debug info in shader
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

        // All resources bound (optimization hint)
        if ((options.flags & ShaderCompileFlags::AllResourcesBound) != ShaderCompileFlags::None)
        {
            arguments.push_back(L"-all_resources_bound");
        }

        // Add defines
        for (const auto& define : options.defines)
        {
            std::wstring defineStr(define.name.begin(), define.name.end());
            defineStr += L"=";
            defineStr += std::wstring(define.value.begin(), define.value.end());
            argumentStorage.push_back(std::move(defineStr));
        }
        for (const auto& defineStr : argumentStorage)
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

        // Enable HLSL 2021 features if using SM 6.6+
        if (model >= ShaderModel::SM_6_6)
        {
            arguments.push_back(L"-HV");
            arguments.push_back(L"2021");
        }

        // Compile the shader
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
            result.errorMessage = "DXC Compile call failed with HRESULT: " + std::to_string(hr);
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
                    ID3D12ShaderReflectionConstantBuffer* cbReflection =
                        reflection->GetConstantBufferByIndex(i);
                    D3D12_SHADER_BUFFER_DESC cbDesc;
                    cbReflection->GetDesc(&cbDesc);

                    ShaderCompileResult::ConstantBufferLayout cb;
                    cb.name = cbDesc.Name;
                    cb.size = cbDesc.Size;

                    // Get binding info
                    for (UINT j = 0; j < shaderDesc.BoundResources; ++j)
                    {
                        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
                        reflection->GetResourceBindingDesc(j, &bindDesc);
                        if (strcmp(bindDesc.Name, cbDesc.Name) == 0)
                        {
                            cb.bindPoint = bindDesc.BindPoint;
                            cb.bindSpace = bindDesc.Space;
                            break;
                        }
                    }

                    // Get variables within the constant buffer
                    for (UINT v = 0; v < cbDesc.Variables; ++v)
                    {
                        ID3D12ShaderReflectionVariable* varReflection =
                            cbReflection->GetVariableByIndex(v);
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

                // Extract all resource bindings
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

        return result;
    }

    ShaderCompileResult ShaderCompiler::CompileFromFile(
        const std::string& filePath,
        const ShaderCompileOptions& options)
    {
        ShaderCompileResult result;

        // Read file contents
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            result.errorMessage = "Failed to open shader file: " + filePath;
            return result;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0);

        std::string source;
        source.resize(fileSize);
        file.read(source.data(), fileSize);
        file.close();

        // Add file directory to include paths
        ShaderCompileOptions modifiedOptions = options;
        std::filesystem::path fsPath(filePath);
        if (fsPath.has_parent_path())
        {
            modifiedOptions.includePaths.push_back(fsPath.parent_path().string());
        }

        return CompileFromSource(source, modifiedOptions);
    }

    void ShaderCompiler::AddIncludePath(const std::string& path)
    {
        m_globalIncludePaths.push_back(path);
    }

    void ShaderCompiler::ClearIncludePaths()
    {
        m_globalIncludePaths.clear();
    }

    // ============================================================================
    // Shader Implementation
    // ============================================================================

    Shader::Shader(ShaderType type, std::vector<uint8_t> bytecode, const std::string& name)
        : m_type(type)
        , m_bytecode(std::move(bytecode))
        , m_name(name)
    {
    }

    D3D12_SHADER_BYTECODE Shader::GetD3D12Bytecode() const
    {
        D3D12_SHADER_BYTECODE bc = {};
        bc.pShaderBytecode = m_bytecode.data();
        bc.BytecodeLength = m_bytecode.size();
        return bc;
    }

    const ShaderCompileResult::ConstantBufferLayout* Shader::GetConstantBuffer(const std::string& name) const
    {
        for (const auto& cb : m_constantBuffers)
        {
            if (cb.name == name)
            {
                return &cb;
            }
        }
        return nullptr;
    }

    void Shader::SetReflectionData(
        std::vector<ShaderCompileResult::ConstantBufferLayout> cbLayouts,
        std::vector<ShaderCompileResult::ResourceBinding> bindings)
    {
        m_constantBuffers = std::move(cbLayouts);
        m_resourceBindings = std::move(bindings);
    }

    // ============================================================================
    // RootSignatureBuilder Implementation
    // ============================================================================

    RootSignatureBuilder& RootSignatureBuilder::AddConstantBufferView(
        uint32_t shaderRegister, uint32_t space, D3D12_SHADER_VISIBILITY visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param.Descriptor.ShaderRegister = shaderRegister;
        param.Descriptor.RegisterSpace = space;
        param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        param.ShaderVisibility = visibility;
        m_parameters.push_back(param);
        return *this;
    }

    RootSignatureBuilder& RootSignatureBuilder::AddShaderResourceView(
        uint32_t shaderRegister, uint32_t space, D3D12_SHADER_VISIBILITY visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        param.Descriptor.ShaderRegister = shaderRegister;
        param.Descriptor.RegisterSpace = space;
        param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        param.ShaderVisibility = visibility;
        m_parameters.push_back(param);
        return *this;
    }

    RootSignatureBuilder& RootSignatureBuilder::AddUnorderedAccessView(
        uint32_t shaderRegister, uint32_t space, D3D12_SHADER_VISIBILITY visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        param.Descriptor.ShaderRegister = shaderRegister;
        param.Descriptor.RegisterSpace = space;
        param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        param.ShaderVisibility = visibility;
        m_parameters.push_back(param);
        return *this;
    }

    RootSignatureBuilder& RootSignatureBuilder::AddRootConstants(
        uint32_t num32BitValues, uint32_t shaderRegister, uint32_t space,
        D3D12_SHADER_VISIBILITY visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.Constants.Num32BitValues = num32BitValues;
        param.Constants.ShaderRegister = shaderRegister;
        param.Constants.RegisterSpace = space;
        param.ShaderVisibility = visibility;
        m_parameters.push_back(param);
        return *this;
    }

    RootSignatureBuilder& RootSignatureBuilder::AddDescriptorTable(
        const std::vector<D3D12_DESCRIPTOR_RANGE1>& ranges,
        D3D12_SHADER_VISIBILITY visibility)
    {
        // Store ranges to keep them alive
        m_descriptorRanges.push_back(ranges);

        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(m_descriptorRanges.back().size());
        param.DescriptorTable.pDescriptorRanges = m_descriptorRanges.back().data();
        param.ShaderVisibility = visibility;
        m_parameters.push_back(param);
        return *this;
    }

    RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(
        uint32_t shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressMode,
        uint32_t space, D3D12_SHADER_VISIBILITY visibility)
    {
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = filter;
        sampler.AddressU = addressMode;
        sampler.AddressV = addressMode;
        sampler.AddressW = addressMode;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = (filter == D3D12_FILTER_ANISOTROPIC) ? 16 : 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = shaderRegister;
        sampler.RegisterSpace = space;
        sampler.ShaderVisibility = visibility;
        m_staticSamplers.push_back(sampler);
        return *this;
    }

    ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(
        ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags)
    {
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.NumParameters = static_cast<UINT>(m_parameters.size());
        desc.Desc_1_1.pParameters = m_parameters.empty() ? nullptr : m_parameters.data();
        desc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(m_staticSamplers.size());
        desc.Desc_1_1.pStaticSamplers = m_staticSamplers.empty() ? nullptr : m_staticSamplers.data();
        desc.Desc_1_1.Flags = flags;

        ComPtr<ID3DBlob> serializedRootSig;
        ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &serializedRootSig, &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                std::cerr << "Root signature serialization error: "
                          << static_cast<const char*>(errorBlob->GetBufferPointer()) << std::endl;
            }
            return nullptr;
        }

        ComPtr<ID3D12RootSignature> rootSignature;
        hr = device->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature));

        if (FAILED(hr))
        {
            std::cerr << "Failed to create root signature" << std::endl;
            return nullptr;
        }

        return rootSignature;
    }

    void RootSignatureBuilder::Reset()
    {
        m_parameters.clear();
        m_staticSamplers.clear();
        m_descriptorRanges.clear();
    }

    // ============================================================================
    // PipelineState Implementation
    // ============================================================================

    PipelineState::PipelineState(ComPtr<ID3D12PipelineState> pso, const std::string& name)
        : m_pso(std::move(pso))
        , m_name(name)
    {
    }

// ============================================================================
// FileWatcher Implementation
// ============================================================================

static std::string NormalizePath(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

FileWatcher::FileWatcher()
{
    m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

    FileWatcher::~FileWatcher()
    {
        Stop();
        if (m_stopEvent)
        {
            CloseHandle(m_stopEvent);
        }
    }

    bool FileWatcher::Start(const std::string& directory, FileChangedCallback callback)
    {
        if (m_running)
        {
            Stop();
        }

        m_directory = directory;
        m_callback = callback;
        m_running = true;

        ResetEvent(m_stopEvent);
        m_watchThread = std::thread(&FileWatcher::WatchThread, this);
        return true;
    }

    void FileWatcher::Stop()
    {
        if (m_running)
        {
            m_running = false;
            SetEvent(m_stopEvent);

            if (m_watchThread.joinable())
            {
                m_watchThread.join();
            }
        }
    }

void FileWatcher::AddFile(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_filesMutex);

    std::string normalized = NormalizePath(filePath);
    if (std::filesystem::exists(normalized))
    {
        m_watchedFiles[normalized] = std::filesystem::last_write_time(normalized);
    }
}

void FileWatcher::RemoveFile(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_filesMutex);
    m_watchedFiles.erase(NormalizePath(filePath));
}

    void FileWatcher::ClearFiles()
    {
        std::lock_guard<std::mutex> lock(m_filesMutex);
        m_watchedFiles.clear();
    }

    void FileWatcher::WatchThread()
    {
        // Use ReadDirectoryChangesW for efficient file watching on Windows
        std::wstring widePath(m_directory.begin(), m_directory.end());

        HANDLE dirHandle = CreateFileW(
            widePath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);

        if (dirHandle == INVALID_HANDLE_VALUE)
        {
            // Fall back to polling if we can't use ReadDirectoryChangesW
            while (m_running)
            {
                std::lock_guard<std::mutex> lock(m_filesMutex);

                for (auto& [path, lastWriteTime] : m_watchedFiles)
                {
                    if (std::filesystem::exists(path))
                    {
                        auto currentTime = std::filesystem::last_write_time(path);
                        if (currentTime != lastWriteTime)
                        {
                            lastWriteTime = currentTime;
                            if (m_callback)
                            {
                                m_callback(path);
                            }
                        }
                    }
                }

                // Wait for stop signal or poll interval
                WaitForSingleObject(m_stopEvent, 500); // Poll every 500ms
            }
            return;
        }

        OVERLAPPED overlapped = {};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        std::vector<BYTE> buffer(4096);
        HANDLE handles[2] = { overlapped.hEvent, m_stopEvent };

        while (m_running)
        {
            DWORD bytesReturned = 0;
            BOOL result = ReadDirectoryChangesW(
                dirHandle,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                TRUE, // Watch subtree
                FILE_NOTIFY_CHANGE_LAST_WRITE,
                &bytesReturned,
                &overlapped,
                nullptr);

            if (!result)
            {
                break;
            }

            DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0)
            {
                // Change detected
                DWORD transferred = 0;
                if (GetOverlappedResult(dirHandle, &overlapped, &transferred, FALSE))
                {
                    FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());

                    while (info)
                    {
                        if (info->Action == FILE_ACTION_MODIFIED)
                        {
                            std::wstring filename(info->FileName, info->FileNameLength / sizeof(WCHAR));
                            std::string narrowFilename(filename.begin(), filename.end());
                            std::string fullPath = m_directory + "/" + narrowFilename;

                            // Normalize path
                            std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

                            // Check if this file is being watched
                            {
                                std::lock_guard<std::mutex> lock(m_filesMutex);
                                auto it = m_watchedFiles.find(fullPath);
                                if (it != m_watchedFiles.end() && m_callback)
                                {
                                    // Small delay to ensure file is fully written
                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                    m_callback(fullPath);
                                    it->second = std::filesystem::last_write_time(fullPath);
                                }
                            }
                        }

                        if (info->NextEntryOffset == 0)
                        {
                            break;
                        }
                        info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                            reinterpret_cast<BYTE*>(info) + info->NextEntryOffset);
                    }
                }
                ResetEvent(overlapped.hEvent);
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                // Stop requested
                break;
            }
        }

        CloseHandle(overlapped.hEvent);
        CloseHandle(dirHandle);
    }

    // ============================================================================
    // ShaderProgram Implementation
    // ============================================================================

    ShaderProgram::ShaderProgram(const std::string& name)
        : m_name(name)
    {
    }

    bool ShaderProgram::LoadVertexShader(ShaderCompiler& compiler, const std::string& path,
        const ShaderCompileOptions& options)
    {
        ShaderCompileOptions opts = options;
        opts.type = ShaderType::Vertex;
        if (opts.entryPoint.empty() || opts.entryPoint == "main")
        {
            opts.entryPoint = "VSMain";
        }

        auto result = compiler.CompileFromFile(path, opts);
        if (!result.success)
        {
            m_lastError = "Vertex shader compilation failed: " + result.errorMessage;
            std::cerr << m_lastError << std::endl;
            return false;
        }

        m_vertexShader = Shader(ShaderType::Vertex, std::move(result.bytecode), path);
        m_vertexShader.SetReflectionData(
            std::move(result.constantBuffers),
            std::move(result.resourceBindings));

        m_vsPath = path;

        if (!result.warningMessage.empty())
        {
            std::cout << "Vertex shader warnings: " << result.warningMessage << std::endl;
        }

        return true;
    }

    bool ShaderProgram::LoadPixelShader(ShaderCompiler& compiler, const std::string& path,
        const ShaderCompileOptions& options)
    {
        ShaderCompileOptions opts = options;
        opts.type = ShaderType::Pixel;
        if (opts.entryPoint.empty() || opts.entryPoint == "main")
        {
            opts.entryPoint = "PSMain";
        }

        auto result = compiler.CompileFromFile(path, opts);
        if (!result.success)
        {
            m_lastError = "Pixel shader compilation failed: " + result.errorMessage;
            std::cerr << m_lastError << std::endl;
            return false;
        }

        m_pixelShader = Shader(ShaderType::Pixel, std::move(result.bytecode), path);
        m_pixelShader.SetReflectionData(
            std::move(result.constantBuffers),
            std::move(result.resourceBindings));

        m_psPath = path;

        if (!result.warningMessage.empty())
        {
            std::cout << "Pixel shader warnings: " << result.warningMessage << std::endl;
        }

        return true;
    }

    bool ShaderProgram::LoadGeometryShader(ShaderCompiler& compiler, const std::string& path,
        const ShaderCompileOptions& options)
    {
        ShaderCompileOptions opts = options;
        opts.type = ShaderType::Geometry;
        if (opts.entryPoint.empty() || opts.entryPoint == "main")
        {
            opts.entryPoint = "GSMain";
        }

        auto result = compiler.CompileFromFile(path, opts);
        if (!result.success)
        {
            m_lastError = "Geometry shader compilation failed: " + result.errorMessage;
            std::cerr << m_lastError << std::endl;
            return false;
        }

        m_geometryShader = Shader(ShaderType::Geometry, std::move(result.bytecode), path);
        m_geometryShader.SetReflectionData(
            std::move(result.constantBuffers),
            std::move(result.resourceBindings));

        m_gsPath = path;
        return true;
    }

    bool ShaderProgram::LoadHullShader(ShaderCompiler& compiler, const std::string& path,
        const ShaderCompileOptions& options)
    {
        ShaderCompileOptions opts = options;
        opts.type = ShaderType::Hull;
        if (opts.entryPoint.empty() || opts.entryPoint == "main")
        {
            opts.entryPoint = "HSMain";
        }

        auto result = compiler.CompileFromFile(path, opts);
        if (!result.success)
        {
            m_lastError = "Hull shader compilation failed: " + result.errorMessage;
            std::cerr << m_lastError << std::endl;
            return false;
        }

        m_hullShader = Shader(ShaderType::Hull, std::move(result.bytecode), path);
        m_hullShader.SetReflectionData(
            std::move(result.constantBuffers),
            std::move(result.resourceBindings));

        m_hsPath = path;
        return true;
    }

    bool ShaderProgram::LoadDomainShader(ShaderCompiler& compiler, const std::string& path,
        const ShaderCompileOptions& options)
    {
        ShaderCompileOptions opts = options;
        opts.type = ShaderType::Domain;
        if (opts.entryPoint.empty() || opts.entryPoint == "main")
        {
            opts.entryPoint = "DSMain";
        }

        auto result = compiler.CompileFromFile(path, opts);
        if (!result.success)
        {
            m_lastError = "Domain shader compilation failed: " + result.errorMessage;
            std::cerr << m_lastError << std::endl;
            return false;
        }

        m_domainShader = Shader(ShaderType::Domain, std::move(result.bytecode), path);
        m_domainShader.SetReflectionData(
            std::move(result.constantBuffers),
            std::move(result.resourceBindings));

        m_dsPath = path;
        return true;
    }

    bool ShaderProgram::LoadComputeShader(ShaderCompiler& compiler, const std::string& path,
        const ShaderCompileOptions& options)
    {
        ShaderCompileOptions opts = options;
        opts.type = ShaderType::Compute;
        if (opts.entryPoint.empty() || opts.entryPoint == "main")
        {
            opts.entryPoint = "CSMain";
        }

        auto result = compiler.CompileFromFile(path, opts);
        if (!result.success)
        {
            m_lastError = "Compute shader compilation failed: " + result.errorMessage;
            std::cerr << m_lastError << std::endl;
            return false;
        }

        m_computeShader = Shader(ShaderType::Compute, std::move(result.bytecode), path);
        m_computeShader.SetReflectionData(
            std::move(result.constantBuffers),
            std::move(result.resourceBindings));

        m_csPath = path;
        return true;
    }

    bool ShaderProgram::CreatePipelineState(ID3D12Device* device, const PipelineStateDesc& desc)
    {
        if (!desc.rootSignature)
        {
            m_lastError = "Root signature is required";
            return false;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        // Root signature
        psoDesc.pRootSignature = desc.rootSignature;
        m_rootSignature = desc.rootSignature;

        // Shaders
        if (desc.vertexShader && desc.vertexShader->IsValid())
        {
            psoDesc.VS = desc.vertexShader->GetD3D12Bytecode();
        }
        else if (m_vertexShader.IsValid())
        {
            psoDesc.VS = m_vertexShader.GetD3D12Bytecode();
        }

        if (desc.pixelShader && desc.pixelShader->IsValid())
        {
            psoDesc.PS = desc.pixelShader->GetD3D12Bytecode();
        }
        else if (m_pixelShader.IsValid())
        {
            psoDesc.PS = m_pixelShader.GetD3D12Bytecode();
        }

        if (desc.geometryShader && desc.geometryShader->IsValid())
        {
            psoDesc.GS = desc.geometryShader->GetD3D12Bytecode();
        }
        else if (m_geometryShader.IsValid())
        {
            psoDesc.GS = m_geometryShader.GetD3D12Bytecode();
        }

        if (desc.hullShader && desc.hullShader->IsValid())
        {
            psoDesc.HS = desc.hullShader->GetD3D12Bytecode();
        }
        else if (m_hullShader.IsValid())
        {
            psoDesc.HS = m_hullShader.GetD3D12Bytecode();
        }

        if (desc.domainShader && desc.domainShader->IsValid())
        {
            psoDesc.DS = desc.domainShader->GetD3D12Bytecode();
        }
        else if (m_domainShader.IsValid())
        {
            psoDesc.DS = m_domainShader.GetD3D12Bytecode();
        }

        // Input layout
        psoDesc.InputLayout.NumElements = static_cast<UINT>(desc.inputLayout.size());
        psoDesc.InputLayout.pInputElementDescs = desc.inputLayout.empty() ? nullptr : desc.inputLayout.data();

        // Rasterizer state
        psoDesc.RasterizerState.FillMode = desc.fillMode;
        psoDesc.RasterizerState.CullMode = desc.cullMode;
        psoDesc.RasterizerState.FrontCounterClockwise = desc.frontCounterClockwise;
        psoDesc.RasterizerState.DepthBias = desc.depthBias;
        psoDesc.RasterizerState.DepthBiasClamp = desc.depthBiasClamp;
        psoDesc.RasterizerState.SlopeScaledDepthBias = desc.slopeScaledDepthBias;
        psoDesc.RasterizerState.DepthClipEnable = desc.depthClipEnable;
        psoDesc.RasterizerState.MultisampleEnable = desc.multisampleEnable;
        psoDesc.RasterizerState.AntialiasedLineEnable = desc.antialiasedLineEnable;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Blend state
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = desc.blendEnable;
        psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = desc.srcBlend;
        psoDesc.BlendState.RenderTarget[0].DestBlend = desc.destBlend;
        psoDesc.BlendState.RenderTarget[0].BlendOp = desc.blendOp;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = desc.srcBlendAlpha;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = desc.destBlendAlpha;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = desc.blendOpAlpha;
        psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = desc.renderTargetWriteMask;

        // Depth stencil state
        psoDesc.DepthStencilState.DepthEnable = desc.depthEnable;
        psoDesc.DepthStencilState.DepthWriteMask = desc.depthWriteEnable ?
            D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = desc.depthFunc;
        psoDesc.DepthStencilState.StencilEnable = desc.stencilEnable;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDesc.DepthStencilState.BackFace = psoDesc.DepthStencilState.FrontFace;

        // Sample desc
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc.Count = desc.sampleCount;
        psoDesc.SampleDesc.Quality = desc.sampleQuality;

        // Primitive topology
        psoDesc.PrimitiveTopologyType = desc.primitiveTopology;

        // Render target formats
        psoDesc.NumRenderTargets = static_cast<UINT>(desc.rtvFormats.size());
        for (size_t i = 0; i < desc.rtvFormats.size() && i < 8; ++i)
        {
            psoDesc.RTVFormats[i] = desc.rtvFormats[i];
        }

        // Depth stencil format
        psoDesc.DSVFormat = desc.dsvFormat;

        // Create PSO
        ComPtr<ID3D12PipelineState> pso;
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
        if (FAILED(hr))
        {
            m_lastError = "Failed to create graphics pipeline state. HRESULT: " + std::to_string(hr);
            return false;
        }

        // Set debug name
        if (!desc.debugName.empty())
        {
            std::wstring wideName(desc.debugName.begin(), desc.debugName.end());
            pso->SetName(wideName.c_str());
        }

        m_pipelineState = PipelineState(std::move(pso), desc.debugName);
        return true;
    }

    bool ShaderProgram::CreateComputePipelineState(ID3D12Device* device, const ComputePipelineStateDesc& desc)
    {
        if (!desc.rootSignature)
        {
            m_lastError = "Root signature is required";
            return false;
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = desc.rootSignature;
        m_rootSignature = desc.rootSignature;

        if (desc.computeShader && desc.computeShader->IsValid())
        {
            psoDesc.CS = desc.computeShader->GetD3D12Bytecode();
        }
        else if (m_computeShader.IsValid())
        {
            psoDesc.CS = m_computeShader.GetD3D12Bytecode();
        }
        else
        {
            m_lastError = "No compute shader available";
            return false;
        }

        ComPtr<ID3D12PipelineState> pso;
        HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso));
        if (FAILED(hr))
        {
            m_lastError = "Failed to create compute pipeline state. HRESULT: " + std::to_string(hr);
            return false;
        }

        if (!desc.debugName.empty())
        {
            std::wstring wideName(desc.debugName.begin(), desc.debugName.end());
            pso->SetName(wideName.c_str());
        }

        m_pipelineState = PipelineState(std::move(pso), desc.debugName);
        return true;
    }

    // ============================================================================
    // ShaderManager Implementation
    // ============================================================================

    ShaderManager::ShaderManager()
    {
    }

    ShaderManager::~ShaderManager()
    {
        Shutdown();
    }

    bool ShaderManager::Initialize(ID3D12Device* device, const std::string& shaderDirectory)
    {
        m_device = device;
        m_shaderDirectory = shaderDirectory;

        if (!m_compiler.Initialize())
        {
            std::cerr << "Failed to initialize shader compiler: " << m_compiler.GetLastError() << std::endl;
            return false;
        }

        // Add shader directory to include paths
        m_compiler.AddIncludePath(shaderDirectory);

        return true;
    }

    void ShaderManager::Shutdown()
    {
        EnableHotReload(false);

        std::lock_guard<std::mutex> lock(m_programsMutex);
        m_programs.clear();

        m_compiler.Shutdown();
        m_device = nullptr;
    }

    ShaderProgram* ShaderManager::GetProgram(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_programsMutex);
        auto it = m_programs.find(name);
        if (it != m_programs.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    ShaderProgram* ShaderManager::CreateProgram(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_programsMutex);

        auto it = m_programs.find(name);
        if (it != m_programs.end())
        {
            return it->second.get();
        }

        auto program = std::make_unique<ShaderProgram>(name);
        ShaderProgram* ptr = program.get();
        m_programs[name] = std::move(program);

        return ptr;
    }

    ShaderProgram* ShaderManager::LoadProgram(
        const std::string& name,
        const std::string& hlslPath,
        const PipelineStateDesc& psoDesc,
        const std::string& vsEntry,
        const std::string& psEntry)
    {
        ShaderProgram* program = CreateProgram(name);
        if (!program)
        {
            return nullptr;
        }

        // Set up compile options
        ShaderCompileOptions vsOptions;
        vsOptions.entryPoint = vsEntry;

        ShaderCompileOptions psOptions;
        psOptions.entryPoint = psEntry;

        // Load shaders
        if (!program->LoadVertexShader(m_compiler, hlslPath, vsOptions))
        {
            std::cerr << "Failed to load vertex shader from " << hlslPath << std::endl;
            return nullptr;
        }

        if (!program->LoadPixelShader(m_compiler, hlslPath, psOptions))
        {
            std::cerr << "Failed to load pixel shader from " << hlslPath << std::endl;
            return nullptr;
        }

        // Create PSO
        if (!program->CreatePipelineState(m_device, psoDesc))
        {
            std::cerr << "Failed to create pipeline state: " << program->GetLastError() << std::endl;
            return nullptr;
        }

        // Register for hot-reload if enabled
        if (m_hotReloadEnabled && m_fileWatcher)
        {
            m_fileWatcher->AddFile(hlslPath);
        }

        return program;
    }

    void ShaderManager::EnableHotReload(bool enable)
    {
        if (enable == m_hotReloadEnabled)
        {
            return;
        }

        m_hotReloadEnabled = enable;

        if (enable)
        {
            m_fileWatcher = std::make_unique<FileWatcher>();
            m_fileWatcher->Start(m_shaderDirectory,
                [this](const std::string& path) { OnFileChanged(path); });

            // Register all currently loaded shader files
            std::lock_guard<std::mutex> lock(m_programsMutex);
            for (const auto& [name, program] : m_programs)
            {
                if (!program->GetVertexShaderPath().empty())
                {
                    m_fileWatcher->AddFile(program->GetVertexShaderPath());
                }
                if (!program->GetPixelShaderPath().empty())
                {
                    m_fileWatcher->AddFile(program->GetPixelShaderPath());
                }
            }
        }
        else
        {
            if (m_fileWatcher)
            {
                m_fileWatcher->Stop();
                m_fileWatcher.reset();
            }
        }
    }

void ShaderManager::OnFileChanged(const std::string& filePath)
{
    std::string normalized = NormalizePath(filePath);
    std::lock_guard<std::mutex> lock(m_dirtyFilesMutex);
    m_dirtyFiles.push_back(normalized);

    // Mark associated programs as dirty
    std::lock_guard<std::mutex> programLock(m_programsMutex);
    for (auto& [name, program] : m_programs)
    {
        if (program->GetVertexShaderPath() == normalized ||
            program->GetPixelShaderPath() == normalized)
        {
            program->MarkDirty();
            std::cout << "Shader program '" << name << "' marked for reload due to: " << normalized << std::endl;
        }
    }
}

    void ShaderManager::CheckForReloads()
    {
        std::vector<std::string> filesToProcess;

        {
            std::lock_guard<std::mutex> lock(m_dirtyFilesMutex);
            filesToProcess = std::move(m_dirtyFiles);
            m_dirtyFiles.clear();
        }

        if (!filesToProcess.empty())
        {
            std::cout << "Detected " << filesToProcess.size() << " shader file(s) changed." << std::endl;
        }
    }

    void ShaderManager::ReloadAllDirty()
    {
        std::lock_guard<std::mutex> lock(m_programsMutex);

        for (auto& [name, program] : m_programs)
        {
            if (program->IsDirty())
            {
                std::cout << "Reloading shader program: " << name << std::endl;

                // Store old PSO info
                auto oldRootSig = program->GetRootSignature();

                // Reload shaders
                bool reloadSuccess = true;

                if (!program->GetVertexShaderPath().empty())
                {
                    ShaderCompileOptions opts;
                    opts.entryPoint = "VSMain";
                    if (!program->LoadVertexShader(m_compiler, program->GetVertexShaderPath(), opts))
                    {
                        std::cerr << "Failed to reload vertex shader" << std::endl;
                        reloadSuccess = false;
                    }
                }

                if (reloadSuccess && !program->GetPixelShaderPath().empty())
                {
                    ShaderCompileOptions opts;
                    opts.entryPoint = "PSMain";
                    if (!program->LoadPixelShader(m_compiler, program->GetPixelShaderPath(), opts))
                    {
                        std::cerr << "Failed to reload pixel shader" << std::endl;
                        reloadSuccess = false;
                    }
                }

                if (reloadSuccess)
                {
                    // Recreate PSO with default settings
                    // In a real implementation, you'd want to store and reuse the original PipelineStateDesc
                    PipelineStateDesc psoDesc;
                    psoDesc.rootSignature = oldRootSig;
                    psoDesc.rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
                    psoDesc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
                    psoDesc.debugName = name;

                    if (program->CreatePipelineState(m_device, psoDesc))
                    {
                        std::cout << "Successfully reloaded shader program: " << name << std::endl;
                    }
                    else
                    {
                        std::cerr << "Failed to recreate PSO: " << program->GetLastError() << std::endl;
                    }
                }

                program->ClearDirty();
            }
        }
    }

} // namespace DX12

#endif // _WIN32
