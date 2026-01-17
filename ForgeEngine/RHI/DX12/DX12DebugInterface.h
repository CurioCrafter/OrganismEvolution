#pragma once

/**
 * DX12 Debug Interface
 *
 * Provides runtime control over DirectX 12 debug output.
 * Integrates with MCP server for external control via Claude.
 *
 * Features:
 * - Debug layer configuration
 * - GPU-based validation control
 * - Message severity/category filtering
 * - DRED (Device Removed Extended Data) configuration
 * - Real-time debug message streaming
 * - Debug marker insertion
 */

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
#include <d3d12sdklayers.h>
#include <wrl/client.h>

#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <mutex>
#include <atomic>

namespace Forge::RHI::DX12
{
    using Microsoft::WRL::ComPtr;

    /**
     * Debug message severity levels (matches D3D12_MESSAGE_SEVERITY)
     */
    enum class DebugSeverity : uint8_t
    {
        Corruption = 0,  // Memory corruption
        Error = 1,       // API usage error
        Warning = 2,     // Potential issue
        Info = 3,        // Informational
        Message = 4      // General diagnostic
    };

    /**
     * Debug message category (matches D3D12_MESSAGE_CATEGORY)
     */
    enum class DebugCategory : uint8_t
    {
        Unknown = 0,
        Miscellaneous,
        Initialization,
        Cleanup,
        Compilation,
        StateCreation,
        StateSetting,
        StateGetting,
        ResourceManipulation,
        Execution,
        Shader
    };

    /**
     * Debug configuration loaded from/saved to JSON
     */
    struct DebugConfig
    {
        bool debugLayerEnabled = true;
        bool gpuValidationEnabled = false;
        bool synchronizedQueueValidation = true;

        // Severity filter (true = show)
        bool showCorruption = true;
        bool showError = true;
        bool showWarning = true;
        bool showInfo = false;
        bool showMessage = false;

        // Break settings
        bool breakOnCorruption = true;
        bool breakOnError = false;
        bool breakOnWarning = false;

        // DRED settings
        bool dredEnabled = false;
        bool autoBreadcrumbs = false;
        bool pageFaultReporting = false;
        bool breadcrumbContext = false;

        // Output settings
        bool outputToFile = true;
        bool outputToConsole = true;
        std::string outputFilePath = "dx12_debug_output.log";

        // Suppressed message IDs
        std::vector<int> suppressedMessageIds;
    };

    /**
     * Debug message callback type
     */
    using DebugMessageCallback = std::function<void(
        DebugSeverity severity,
        DebugCategory category,
        int messageId,
        const char* description
    )>;

    /**
     * DX12 Debug Interface
     *
     * Singleton class for managing D3D12 debug output.
     * Call Initialize() before device creation.
     */
    class DX12DebugInterface
    {
    public:
        static DX12DebugInterface& Get()
        {
            static DX12DebugInterface instance;
            return instance;
        }

        /**
         * Initialize debug interface (call before D3D12CreateDevice)
         */
        bool Initialize(const std::string& configPath = "dx12_debug_config.json");

        /**
         * Shutdown and cleanup
         */
        void Shutdown();

        /**
         * Attach to device after creation for InfoQueue access
         */
        bool AttachToDevice(ID3D12Device* device);

        /**
         * Process commands from MCP server
         */
        void ProcessCommands(const std::string& commandFilePath = "dx12_debug_command.txt");

        /**
         * Flush any pending debug messages
         */
        void FlushMessages();

        /**
         * Get current configuration
         */
        const DebugConfig& GetConfig() const { return m_config; }

        /**
         * Update configuration and apply changes
         */
        void SetConfig(const DebugConfig& config);

        /**
         * Save current configuration to file
         */
        bool SaveConfig(const std::string& path = "");

        /**
         * Load configuration from file
         */
        bool LoadConfig(const std::string& path = "");

        // === Severity Filter ===

        void SetSeverityFilter(DebugSeverity severity, bool show);
        bool GetSeverityFilter(DebugSeverity severity) const;

        // === Break Settings ===

        void SetBreakOnSeverity(DebugSeverity severity, bool enabled);
        bool GetBreakOnSeverity(DebugSeverity severity) const;

        // === Message Suppression ===

        void SuppressMessage(int messageId);
        void UnsuppressMessage(int messageId);
        void ClearSuppressedMessages();

        // === Debug Markers ===

        void BeginEvent(ID3D12GraphicsCommandList* cmdList, const char* name, uint32_t color = 0xFFFFFFFF);
        void EndEvent(ID3D12GraphicsCommandList* cmdList);
        void SetMarker(ID3D12GraphicsCommandList* cmdList, const char* name, uint32_t color = 0xFFFFFFFF);

        // === Callbacks ===

        void SetMessageCallback(DebugMessageCallback callback) { m_callback = callback; }

        // === Device Info ===

        void DumpDeviceInfo(const std::string& outputPath = "dx12_device_info.txt");

        // === GPU Capture ===

        void TriggerCapture(int frameCount = 1);

        // === Statistics ===

        uint64_t GetTotalMessageCount() const { return m_totalMessageCount; }
        uint64_t GetErrorCount() const { return m_errorCount; }
        uint64_t GetWarningCount() const { return m_warningCount; }

    private:
        DX12DebugInterface() = default;
        ~DX12DebugInterface() { Shutdown(); }

        DX12DebugInterface(const DX12DebugInterface&) = delete;
        DX12DebugInterface& operator=(const DX12DebugInterface&) = delete;

        void ApplyInfoQueueFilters();
        void WriteToLog(DebugSeverity severity, const char* message);
        static const char* SeverityToString(DebugSeverity severity);
        static const char* CategoryToString(DebugCategory category);

        // Configuration
        DebugConfig m_config;
        std::string m_configPath;

        // D3D12 interfaces
        ComPtr<ID3D12Debug> m_debugController;
        ComPtr<ID3D12Debug1> m_debugController1;
        ComPtr<ID3D12Debug3> m_debugController3;
        ComPtr<ID3D12InfoQueue> m_infoQueue;
        ComPtr<ID3D12InfoQueue1> m_infoQueue1;
        ID3D12Device* m_device = nullptr;

        // Message callback
        DebugMessageCallback m_callback;
        DWORD m_callbackCookie = 0;

        // Output file
        std::ofstream m_logFile;
        std::mutex m_logMutex;

        // Statistics
        std::atomic<uint64_t> m_totalMessageCount{0};
        std::atomic<uint64_t> m_errorCount{0};
        std::atomic<uint64_t> m_warningCount{0};

        bool m_initialized = false;
    };

    // === Convenience Macros ===

    #ifdef FORGE_DEBUG
        #define DX12_DEBUG_INIT(configPath) \
            Forge::RHI::DX12::DX12DebugInterface::Get().Initialize(configPath)

        #define DX12_DEBUG_ATTACH(device) \
            Forge::RHI::DX12::DX12DebugInterface::Get().AttachToDevice(device)

        #define DX12_DEBUG_PROCESS_COMMANDS() \
            Forge::RHI::DX12::DX12DebugInterface::Get().ProcessCommands()

        #define DX12_DEBUG_BEGIN_EVENT(cmdList, name) \
            Forge::RHI::DX12::DX12DebugInterface::Get().BeginEvent(cmdList, name)

        #define DX12_DEBUG_END_EVENT(cmdList) \
            Forge::RHI::DX12::DX12DebugInterface::Get().EndEvent(cmdList)

        #define DX12_DEBUG_MARKER(cmdList, name) \
            Forge::RHI::DX12::DX12DebugInterface::Get().SetMarker(cmdList, name)
    #else
        #define DX12_DEBUG_INIT(configPath) ((void)0)
        #define DX12_DEBUG_ATTACH(device) ((void)0)
        #define DX12_DEBUG_PROCESS_COMMANDS() ((void)0)
        #define DX12_DEBUG_BEGIN_EVENT(cmdList, name) ((void)0)
        #define DX12_DEBUG_END_EVENT(cmdList) ((void)0)
        #define DX12_DEBUG_MARKER(cmdList, name) ((void)0)
    #endif

} // namespace Forge::RHI::DX12

#endif // FORGE_RHI_DX12
