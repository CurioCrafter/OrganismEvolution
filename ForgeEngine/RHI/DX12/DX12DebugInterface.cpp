#include "DX12DebugInterface.h"

#if FORGE_RHI_DX12

#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>

// For PIX markers
#ifdef USE_PIX
#include <pix3.h>
#endif

namespace Forge::RHI::DX12
{
    // Simple JSON parser/writer for config (no external dependencies)
    namespace JsonHelper
    {
        std::string Escape(const std::string& str)
        {
            std::string result;
            for (char c : str)
            {
                switch (c)
                {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += c;
                }
            }
            return result;
        }

        bool ParseBool(const std::string& json, const std::string& key, bool defaultValue)
        {
            size_t pos = json.find("\"" + key + "\"");
            if (pos == std::string::npos) return defaultValue;

            pos = json.find(':', pos);
            if (pos == std::string::npos) return defaultValue;

            size_t start = json.find_first_not_of(" \t\n\r", pos + 1);
            if (start == std::string::npos) return defaultValue;

            if (json.substr(start, 4) == "true") return true;
            if (json.substr(start, 5) == "false") return false;
            return defaultValue;
        }

        std::string ParseString(const std::string& json, const std::string& key, const std::string& defaultValue)
        {
            size_t pos = json.find("\"" + key + "\"");
            if (pos == std::string::npos) return defaultValue;

            pos = json.find(':', pos);
            if (pos == std::string::npos) return defaultValue;

            size_t start = json.find('"', pos + 1);
            if (start == std::string::npos) return defaultValue;

            size_t end = json.find('"', start + 1);
            if (end == std::string::npos) return defaultValue;

            return json.substr(start + 1, end - start - 1);
        }
    }

    bool DX12DebugInterface::Initialize(const std::string& configPath)
    {
        if (m_initialized)
            return true;

        m_configPath = configPath.empty() ? "dx12_debug_config.json" : configPath;

        // Load configuration
        LoadConfig(m_configPath);

        // Enable debug layer if configured
        if (m_config.debugLayerEnabled)
        {
            HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController));
            if (SUCCEEDED(hr))
            {
                m_debugController->EnableDebugLayer();
                OutputDebugStringA("[DX12 Debug] Debug layer enabled\n");

                // Enable GPU-based validation if configured
                if (m_config.gpuValidationEnabled)
                {
                    if (SUCCEEDED(m_debugController.As(&m_debugController1)))
                    {
                        m_debugController1->SetEnableGPUBasedValidation(TRUE);
                        m_debugController1->SetEnableSynchronizedCommandQueueValidation(
                            m_config.synchronizedQueueValidation);
                        OutputDebugStringA("[DX12 Debug] GPU-based validation enabled\n");
                    }
                }

                // Get Debug3 interface for additional features
                m_debugController.As(&m_debugController3);
            }
            else
            {
                OutputDebugStringA("[DX12 Debug] Failed to get debug interface. "
                                   "Install Graphics Tools optional feature.\n");
            }
        }

        // Configure DRED if enabled
        if (m_config.dredEnabled)
        {
            ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
            {
                if (m_config.autoBreadcrumbs)
                {
                    dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                }
                if (m_config.pageFaultReporting)
                {
                    dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                }
                if (m_config.breadcrumbContext)
                {
                    dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                }
                OutputDebugStringA("[DX12 Debug] DRED configured\n");
            }
        }

        // Open log file if configured
        if (m_config.outputToFile)
        {
            m_logFile.open(m_config.outputFilePath, std::ios::out | std::ios::trunc);
            if (m_logFile.is_open())
            {
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                m_logFile << "=== DX12 Debug Log Started at " << std::ctime(&time) << "===\n\n";
            }
        }

        m_initialized = true;
        return true;
    }

    void DX12DebugInterface::Shutdown()
    {
        if (!m_initialized)
            return;

        // Unregister message callback
        if (m_infoQueue1 && m_callbackCookie != 0)
        {
            m_infoQueue1->UnregisterMessageCallback(m_callbackCookie);
            m_callbackCookie = 0;
        }

        // Close log file
        if (m_logFile.is_open())
        {
            m_logFile << "\n=== DX12 Debug Log Ended ===\n";
            m_logFile.close();
        }

        // Release interfaces
        m_infoQueue1.Reset();
        m_infoQueue.Reset();
        m_debugController3.Reset();
        m_debugController1.Reset();
        m_debugController.Reset();
        m_device = nullptr;

        m_initialized = false;
    }

    bool DX12DebugInterface::AttachToDevice(ID3D12Device* device)
    {
        if (!device)
            return false;

        m_device = device;

        // Get InfoQueue interface
        HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&m_infoQueue));
        if (FAILED(hr))
        {
            OutputDebugStringA("[DX12 Debug] Failed to get ID3D12InfoQueue\n");
            return false;
        }

        // Try to get InfoQueue1 for message callbacks
        device->QueryInterface(IID_PPV_ARGS(&m_infoQueue1));

        // Apply initial filters
        ApplyInfoQueueFilters();

        // Register message callback if InfoQueue1 is available
        if (m_infoQueue1)
        {
            auto callbackLambda = [](
                D3D12_MESSAGE_CATEGORY category,
                D3D12_MESSAGE_SEVERITY severity,
                D3D12_MESSAGE_ID id,
                LPCSTR description,
                void* context)
            {
                auto* self = static_cast<DX12DebugInterface*>(context);

                // Update statistics
                self->m_totalMessageCount++;
                if (severity == D3D12_MESSAGE_SEVERITY_ERROR ||
                    severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
                {
                    self->m_errorCount++;
                }
                else if (severity == D3D12_MESSAGE_SEVERITY_WARNING)
                {
                    self->m_warningCount++;
                }

                // Write to log
                self->WriteToLog(static_cast<DebugSeverity>(severity), description);

                // Call user callback if set
                if (self->m_callback)
                {
                    self->m_callback(
                        static_cast<DebugSeverity>(severity),
                        static_cast<DebugCategory>(category),
                        static_cast<int>(id),
                        description);
                }
            };

            hr = m_infoQueue1->RegisterMessageCallback(
                callbackLambda,
                D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                this,
                &m_callbackCookie);

            if (SUCCEEDED(hr))
            {
                OutputDebugStringA("[DX12 Debug] Message callback registered\n");
            }
        }

        OutputDebugStringA("[DX12 Debug] Attached to device\n");
        return true;
    }

    void DX12DebugInterface::ProcessCommands(const std::string& commandFilePath)
    {
        if (!std::filesystem::exists(commandFilePath))
            return;

        std::ifstream file(commandFilePath);
        if (!file.is_open())
            return;

        std::string command;
        while (std::getline(file, command))
        {
            if (command.empty())
                continue;

            // Parse and execute command
            if (command == "reload_debug_config")
            {
                LoadConfig(m_configPath);
                ApplyInfoQueueFilters();
            }
            else if (command == "update_severity_filter")
            {
                LoadConfig(m_configPath);
                ApplyInfoQueueFilters();
            }
            else if (command == "update_category_filter")
            {
                LoadConfig(m_configPath);
                ApplyInfoQueueFilters();
            }
            else if (command == "update_break_settings")
            {
                LoadConfig(m_configPath);
                ApplyInfoQueueFilters();
            }
            else if (command == "clear_debug_messages")
            {
                if (m_infoQueue)
                {
                    m_infoQueue->ClearStoredMessages();
                }
                // Clear log file
                if (m_logFile.is_open())
                {
                    m_logFile.close();
                    m_logFile.open(m_config.outputFilePath, std::ios::out | std::ios::trunc);
                }
            }
            else if (command == "dump_device_info")
            {
                DumpDeviceInfo();
            }
            else if (command.rfind("suppress_message ", 0) == 0)
            {
                // Parse: suppress_message <id> <0|1>
                int id, suppress;
                if (sscanf(command.c_str(), "suppress_message %d %d", &id, &suppress) == 2)
                {
                    if (suppress)
                        SuppressMessage(id);
                    else
                        UnsuppressMessage(id);
                }
            }
            else if (command.rfind("capture_frames ", 0) == 0)
            {
                int count = 1;
                sscanf(command.c_str(), "capture_frames %d", &count);
                TriggerCapture(count);
            }
            else if (command.rfind("insert_marker ", 0) == 0)
            {
                // Parse: insert_marker <name> <color_hex>
                char name[256] = {0};
                char colorStr[16] = {0};
                if (sscanf(command.c_str(), "insert_marker %255s %15s", name, colorStr) >= 1)
                {
                    uint32_t color = 0xFFFFFFFF;
                    if (strlen(colorStr) > 0)
                    {
                        color = (uint32_t)strtoul(colorStr, nullptr, 16);
                    }
                    // Note: Would need active command list to insert marker
                    WriteToLog(DebugSeverity::Info, (std::string("Marker: ") + name).c_str());
                }
            }
        }

        file.close();

        // Clear the command file
        std::ofstream clearFile(commandFilePath, std::ios::out | std::ios::trunc);
        clearFile.close();
    }

    void DX12DebugInterface::FlushMessages()
    {
        if (!m_infoQueue)
            return;

        UINT64 messageCount = m_infoQueue->GetNumStoredMessages();

        for (UINT64 i = 0; i < messageCount; ++i)
        {
            SIZE_T messageLength = 0;
            m_infoQueue->GetMessage(i, nullptr, &messageLength);

            if (messageLength == 0)
                continue;

            std::vector<char> messageBuffer(messageLength);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(messageBuffer.data());

            if (SUCCEEDED(m_infoQueue->GetMessage(i, message, &messageLength)))
            {
                WriteToLog(static_cast<DebugSeverity>(message->Severity), message->pDescription);

                if (m_callback)
                {
                    m_callback(
                        static_cast<DebugSeverity>(message->Severity),
                        static_cast<DebugCategory>(message->Category),
                        static_cast<int>(message->ID),
                        message->pDescription);
                }
            }
        }

        m_infoQueue->ClearStoredMessages();
    }

    void DX12DebugInterface::SetConfig(const DebugConfig& config)
    {
        m_config = config;
        ApplyInfoQueueFilters();
    }

    bool DX12DebugInterface::SaveConfig(const std::string& path)
    {
        std::string filePath = path.empty() ? m_configPath : path;

        std::ofstream file(filePath);
        if (!file.is_open())
            return false;

        file << "{\n";
        file << "  \"debugLayerEnabled\": " << (m_config.debugLayerEnabled ? "true" : "false") << ",\n";
        file << "  \"gpuValidationEnabled\": " << (m_config.gpuValidationEnabled ? "true" : "false") << ",\n";
        file << "  \"synchronizedQueueValidation\": " << (m_config.synchronizedQueueValidation ? "true" : "false") << ",\n";

        file << "  \"severityFilter\": {\n";
        file << "    \"corruption\": " << (m_config.showCorruption ? "true" : "false") << ",\n";
        file << "    \"error\": " << (m_config.showError ? "true" : "false") << ",\n";
        file << "    \"warning\": " << (m_config.showWarning ? "true" : "false") << ",\n";
        file << "    \"info\": " << (m_config.showInfo ? "true" : "false") << ",\n";
        file << "    \"message\": " << (m_config.showMessage ? "true" : "false") << "\n";
        file << "  },\n";

        file << "  \"breakOnCorruption\": " << (m_config.breakOnCorruption ? "true" : "false") << ",\n";
        file << "  \"breakOnError\": " << (m_config.breakOnError ? "true" : "false") << ",\n";
        file << "  \"breakOnWarning\": " << (m_config.breakOnWarning ? "true" : "false") << ",\n";

        file << "  \"dredEnabled\": " << (m_config.dredEnabled ? "true" : "false") << ",\n";
        file << "  \"autoBreadcrumbs\": " << (m_config.autoBreadcrumbs ? "true" : "false") << ",\n";
        file << "  \"pageFaultReporting\": " << (m_config.pageFaultReporting ? "true" : "false") << ",\n";
        file << "  \"breadcrumbContext\": " << (m_config.breadcrumbContext ? "true" : "false") << ",\n";

        file << "  \"outputToFile\": " << (m_config.outputToFile ? "true" : "false") << ",\n";
        file << "  \"outputToConsole\": " << (m_config.outputToConsole ? "true" : "false") << ",\n";
        file << "  \"outputFilePath\": \"" << JsonHelper::Escape(m_config.outputFilePath) << "\",\n";

        file << "  \"suppressedMessageIds\": [";
        for (size_t i = 0; i < m_config.suppressedMessageIds.size(); ++i)
        {
            if (i > 0) file << ", ";
            file << m_config.suppressedMessageIds[i];
        }
        file << "]\n";

        file << "}\n";

        return true;
    }

    bool DX12DebugInterface::LoadConfig(const std::string& path)
    {
        std::string filePath = path.empty() ? m_configPath : path;

        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();

        // Parse JSON (simple parser)
        m_config.debugLayerEnabled = JsonHelper::ParseBool(json, "debugLayerEnabled", true);
        m_config.gpuValidationEnabled = JsonHelper::ParseBool(json, "gpuValidationEnabled", false);
        m_config.synchronizedQueueValidation = JsonHelper::ParseBool(json, "synchronizedQueueValidation", true);

        // Parse severity filter from nested object
        size_t severityPos = json.find("\"severityFilter\"");
        if (severityPos != std::string::npos)
        {
            size_t start = json.find('{', severityPos);
            size_t end = json.find('}', start);
            if (start != std::string::npos && end != std::string::npos)
            {
                std::string severityJson = json.substr(start, end - start + 1);
                m_config.showCorruption = JsonHelper::ParseBool(severityJson, "corruption", true);
                m_config.showError = JsonHelper::ParseBool(severityJson, "error", true);
                m_config.showWarning = JsonHelper::ParseBool(severityJson, "warning", true);
                m_config.showInfo = JsonHelper::ParseBool(severityJson, "info", false);
                m_config.showMessage = JsonHelper::ParseBool(severityJson, "message", false);
            }
        }

        m_config.breakOnCorruption = JsonHelper::ParseBool(json, "breakOnCorruption", true);
        m_config.breakOnError = JsonHelper::ParseBool(json, "breakOnError", false);
        m_config.breakOnWarning = JsonHelper::ParseBool(json, "breakOnWarning", false);

        m_config.dredEnabled = JsonHelper::ParseBool(json, "dredEnabled", false);
        m_config.autoBreadcrumbs = JsonHelper::ParseBool(json, "autoBreadcrumbs", false);
        m_config.pageFaultReporting = JsonHelper::ParseBool(json, "pageFaultReporting", false);
        m_config.breadcrumbContext = JsonHelper::ParseBool(json, "breadcrumbContext", false);

        m_config.outputToFile = JsonHelper::ParseBool(json, "outputToFile", true);
        m_config.outputToConsole = JsonHelper::ParseBool(json, "outputToConsole", true);
        m_config.outputFilePath = JsonHelper::ParseString(json, "outputFilePath", "dx12_debug_output.log");

        return true;
    }

    void DX12DebugInterface::SetSeverityFilter(DebugSeverity severity, bool show)
    {
        switch (severity)
        {
            case DebugSeverity::Corruption: m_config.showCorruption = show; break;
            case DebugSeverity::Error: m_config.showError = show; break;
            case DebugSeverity::Warning: m_config.showWarning = show; break;
            case DebugSeverity::Info: m_config.showInfo = show; break;
            case DebugSeverity::Message: m_config.showMessage = show; break;
        }
        ApplyInfoQueueFilters();
    }

    bool DX12DebugInterface::GetSeverityFilter(DebugSeverity severity) const
    {
        switch (severity)
        {
            case DebugSeverity::Corruption: return m_config.showCorruption;
            case DebugSeverity::Error: return m_config.showError;
            case DebugSeverity::Warning: return m_config.showWarning;
            case DebugSeverity::Info: return m_config.showInfo;
            case DebugSeverity::Message: return m_config.showMessage;
        }
        return true;
    }

    void DX12DebugInterface::SetBreakOnSeverity(DebugSeverity severity, bool enabled)
    {
        if (!m_infoQueue)
            return;

        D3D12_MESSAGE_SEVERITY d3dSeverity;
        switch (severity)
        {
            case DebugSeverity::Corruption:
                d3dSeverity = D3D12_MESSAGE_SEVERITY_CORRUPTION;
                m_config.breakOnCorruption = enabled;
                break;
            case DebugSeverity::Error:
                d3dSeverity = D3D12_MESSAGE_SEVERITY_ERROR;
                m_config.breakOnError = enabled;
                break;
            case DebugSeverity::Warning:
                d3dSeverity = D3D12_MESSAGE_SEVERITY_WARNING;
                m_config.breakOnWarning = enabled;
                break;
            default:
                return;
        }

        m_infoQueue->SetBreakOnSeverity(d3dSeverity, enabled ? TRUE : FALSE);
    }

    bool DX12DebugInterface::GetBreakOnSeverity(DebugSeverity severity) const
    {
        switch (severity)
        {
            case DebugSeverity::Corruption: return m_config.breakOnCorruption;
            case DebugSeverity::Error: return m_config.breakOnError;
            case DebugSeverity::Warning: return m_config.breakOnWarning;
            default: return false;
        }
    }

    void DX12DebugInterface::SuppressMessage(int messageId)
    {
        if (std::find(m_config.suppressedMessageIds.begin(),
                      m_config.suppressedMessageIds.end(),
                      messageId) == m_config.suppressedMessageIds.end())
        {
            m_config.suppressedMessageIds.push_back(messageId);
            ApplyInfoQueueFilters();
        }
    }

    void DX12DebugInterface::UnsuppressMessage(int messageId)
    {
        auto it = std::find(m_config.suppressedMessageIds.begin(),
                            m_config.suppressedMessageIds.end(),
                            messageId);
        if (it != m_config.suppressedMessageIds.end())
        {
            m_config.suppressedMessageIds.erase(it);
            ApplyInfoQueueFilters();
        }
    }

    void DX12DebugInterface::ClearSuppressedMessages()
    {
        m_config.suppressedMessageIds.clear();
        ApplyInfoQueueFilters();
    }

    void DX12DebugInterface::BeginEvent(ID3D12GraphicsCommandList* cmdList, const char* name, uint32_t color)
    {
        if (!cmdList)
            return;

#ifdef USE_PIX
        PIXBeginEvent(cmdList, color, name);
#else
        // Use ID3D12GraphicsCommandList::BeginEvent if available
        (void)name;
        (void)color;
#endif
    }

    void DX12DebugInterface::EndEvent(ID3D12GraphicsCommandList* cmdList)
    {
        if (!cmdList)
            return;

#ifdef USE_PIX
        PIXEndEvent(cmdList);
#endif
    }

    void DX12DebugInterface::SetMarker(ID3D12GraphicsCommandList* cmdList, const char* name, uint32_t color)
    {
        if (!cmdList)
            return;

#ifdef USE_PIX
        PIXSetMarker(cmdList, color, name);
#else
        (void)name;
        (void)color;
#endif
    }

    void DX12DebugInterface::DumpDeviceInfo(const std::string& outputPath)
    {
        std::ofstream file(outputPath);
        if (!file.is_open())
            return;

        file << "=== D3D12 Device Information ===\n\n";

        if (m_device)
        {
            // Get adapter info via DXGI
            ComPtr<IDXGIFactory4> factory;
            ComPtr<IDXGIAdapter1> adapter;

            if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
            {
                LUID luid = m_device->GetAdapterLuid();

                for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    adapter->GetDesc1(&desc);

                    if (desc.AdapterLuid.LowPart == luid.LowPart &&
                        desc.AdapterLuid.HighPart == luid.HighPart)
                    {
                        char nameBuf[256];
                        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);

                        file << "Device: " << nameBuf << "\n";
                        file << "Vendor ID: 0x" << std::hex << desc.VendorId << std::dec << "\n";
                        file << "Device ID: 0x" << std::hex << desc.DeviceId << std::dec << "\n";
                        file << "Dedicated Video Memory: " << (desc.DedicatedVideoMemory / (1024 * 1024)) << " MB\n";
                        file << "Dedicated System Memory: " << (desc.DedicatedSystemMemory / (1024 * 1024)) << " MB\n";
                        file << "Shared System Memory: " << (desc.SharedSystemMemory / (1024 * 1024)) << " MB\n";
                        break;
                    }
                }
            }

            // Feature level
            D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
            D3D_FEATURE_LEVEL requestedLevels[] = {
                D3D_FEATURE_LEVEL_12_2,
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0
            };
            featureLevels.NumFeatureLevels = _countof(requestedLevels);
            featureLevels.pFeatureLevelsRequested = requestedLevels;

            if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels))))
            {
                file << "\nMax Feature Level: ";
                switch (featureLevels.MaxSupportedFeatureLevel)
                {
                    case D3D_FEATURE_LEVEL_12_2: file << "12.2"; break;
                    case D3D_FEATURE_LEVEL_12_1: file << "12.1"; break;
                    case D3D_FEATURE_LEVEL_12_0: file << "12.0"; break;
                    case D3D_FEATURE_LEVEL_11_1: file << "11.1"; break;
                    case D3D_FEATURE_LEVEL_11_0: file << "11.0"; break;
                    default: file << "Unknown"; break;
                }
                file << "\n";
            }

            // Raytracing support
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
            {
                file << "\nRaytracing Tier: ";
                switch (options5.RaytracingTier)
                {
                    case D3D12_RAYTRACING_TIER_NOT_SUPPORTED: file << "Not Supported"; break;
                    case D3D12_RAYTRACING_TIER_1_0: file << "1.0"; break;
                    case D3D12_RAYTRACING_TIER_1_1: file << "1.1"; break;
                    default: file << "Unknown"; break;
                }
                file << "\n";
            }

            // Mesh shader support
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
            if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
            {
                file << "Mesh Shader Tier: ";
                switch (options7.MeshShaderTier)
                {
                    case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED: file << "Not Supported"; break;
                    case D3D12_MESH_SHADER_TIER_1: file << "1"; break;
                    default: file << "Unknown"; break;
                }
                file << "\n";
            }
        }

        file << "\n=== Debug Configuration ===\n";
        file << "Debug Layer: " << (m_config.debugLayerEnabled ? "Enabled" : "Disabled") << "\n";
        file << "GPU Validation: " << (m_config.gpuValidationEnabled ? "Enabled" : "Disabled") << "\n";
        file << "DRED: " << (m_config.dredEnabled ? "Enabled" : "Disabled") << "\n";

        file << "\n=== Statistics ===\n";
        file << "Total Messages: " << m_totalMessageCount.load() << "\n";
        file << "Errors: " << m_errorCount.load() << "\n";
        file << "Warnings: " << m_warningCount.load() << "\n";
    }

    void DX12DebugInterface::TriggerCapture(int frameCount)
    {
#ifdef USE_PIX
        for (int i = 0; i < frameCount; ++i)
        {
            PIXGpuCaptureNextFrames(L"capture.wpix", 1);
        }
#else
        (void)frameCount;
        WriteToLog(DebugSeverity::Info, "GPU capture requested but PIX not available");
#endif
    }

    void DX12DebugInterface::ApplyInfoQueueFilters()
    {
        if (!m_infoQueue)
            return;

        // Clear existing filters
        m_infoQueue->ClearStorageFilter();
        m_infoQueue->ClearRetrievalFilter();

        // Build deny list for severities
        std::vector<D3D12_MESSAGE_SEVERITY> denySeverities;
        if (!m_config.showMessage) denySeverities.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);
        if (!m_config.showInfo) denySeverities.push_back(D3D12_MESSAGE_SEVERITY_INFO);
        if (!m_config.showWarning) denySeverities.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
        if (!m_config.showError) denySeverities.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
        // Never filter corruption by default

        // Build deny list for message IDs
        std::vector<D3D12_MESSAGE_ID> denyIds;
        for (int id : m_config.suppressedMessageIds)
        {
            denyIds.push_back(static_cast<D3D12_MESSAGE_ID>(id));
        }

        // Apply filter
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = static_cast<UINT>(denySeverities.size());
        filter.DenyList.pSeverityList = denySeverities.empty() ? nullptr : denySeverities.data();
        filter.DenyList.NumIDs = static_cast<UINT>(denyIds.size());
        filter.DenyList.pIDList = denyIds.empty() ? nullptr : denyIds.data();

        m_infoQueue->AddStorageFilterEntries(&filter);

        // Apply break settings
        m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, m_config.breakOnCorruption ? TRUE : FALSE);
        m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, m_config.breakOnError ? TRUE : FALSE);
        m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, m_config.breakOnWarning ? TRUE : FALSE);
    }

    void DX12DebugInterface::WriteToLog(DebugSeverity severity, const char* message)
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        // Get timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm_buf;
        localtime_s(&tm_buf, &time);

        std::stringstream timestamp;
        timestamp << std::put_time(&tm_buf, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();

        // Format message
        std::string formatted = "[" + timestamp.str() + "] [" + SeverityToString(severity) + "] " + message + "\n";

        // Write to file
        if (m_config.outputToFile && m_logFile.is_open())
        {
            m_logFile << formatted;
            m_logFile.flush();
        }

        // Write to debug console
        if (m_config.outputToConsole)
        {
            OutputDebugStringA(formatted.c_str());
        }
    }

    const char* DX12DebugInterface::SeverityToString(DebugSeverity severity)
    {
        switch (severity)
        {
            case DebugSeverity::Corruption: return "CORRUPTION";
            case DebugSeverity::Error: return "ERROR";
            case DebugSeverity::Warning: return "WARNING";
            case DebugSeverity::Info: return "INFO";
            case DebugSeverity::Message: return "MESSAGE";
            default: return "UNKNOWN";
        }
    }

    const char* DX12DebugInterface::CategoryToString(DebugCategory category)
    {
        switch (category)
        {
            case DebugCategory::Unknown: return "Unknown";
            case DebugCategory::Miscellaneous: return "Miscellaneous";
            case DebugCategory::Initialization: return "Initialization";
            case DebugCategory::Cleanup: return "Cleanup";
            case DebugCategory::Compilation: return "Compilation";
            case DebugCategory::StateCreation: return "StateCreation";
            case DebugCategory::StateSetting: return "StateSetting";
            case DebugCategory::StateGetting: return "StateGetting";
            case DebugCategory::ResourceManipulation: return "ResourceManipulation";
            case DebugCategory::Execution: return "Execution";
            case DebugCategory::Shader: return "Shader";
            default: return "Unknown";
        }
    }

} // namespace Forge::RHI::DX12

#endif // FORGE_RHI_DX12
