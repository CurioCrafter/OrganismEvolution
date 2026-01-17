#pragma once

// Forge Engine - File Watcher
// Monitors directories for file changes (create, modify, delete)

#include "Core/CoreMinimal.h"
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

namespace Forge::Platform
{
    // ========================================================================
    // File Change Types
    // ========================================================================

    enum class FileChangeType : u8
    {
        Created,    // File was created
        Modified,   // File was modified
        Deleted,    // File was deleted
        Renamed     // File was renamed
    };

    /// Get string name for file change type
    [[nodiscard]] constexpr StringView GetFileChangeTypeName(FileChangeType type) noexcept
    {
        switch (type)
        {
            case FileChangeType::Created:  return "Created";
            case FileChangeType::Modified: return "Modified";
            case FileChangeType::Deleted:  return "Deleted";
            case FileChangeType::Renamed:  return "Renamed";
            default:                       return "Unknown";
        }
    }

    // ========================================================================
    // File Change Event
    // ========================================================================

    /// Information about a file change
    struct FileChangeEvent
    {
        std::filesystem::path path;     // Full path to the file
        std::filesystem::path oldPath;  // Old path (for renames)
        FileChangeType type;            // Type of change
        bool isDirectory;               // True if change is to a directory
    };

    // ========================================================================
    // File Watcher Configuration
    // ========================================================================

    struct FileWatcherConfig
    {
        std::filesystem::path watchPath;                // Directory to watch
        bool recursive = true;                          // Watch subdirectories
        u32 pollIntervalMs = 100;                       // Poll interval (milliseconds)
        bool watchFiles = true;                         // Watch file changes
        bool watchDirectories = false;                  // Watch directory changes
        Vector<std::string> extensionFilter;            // Only watch these extensions (empty = all)
    };

    // ========================================================================
    // File Watcher
    // ========================================================================

    /// Callback for file changes
    using FileChangeCallback = Function<void(const FileChangeEvent& event)>;

    /// File watcher that monitors a directory for changes
    /// Uses ReadDirectoryChangesW on Windows for efficient monitoring
    class FileWatcher : public NonCopyable
    {
    public:
        FileWatcher() = default;
        ~FileWatcher();

        /// Initialize the watcher
        /// @param config Configuration for the watcher
        /// @return True if initialization succeeded
        bool Initialize(const FileWatcherConfig& config);

        /// Shutdown the watcher
        void Shutdown();

        /// Check if the watcher is running
        [[nodiscard]] bool IsRunning() const { return m_running.load(); }

        /// Start watching for changes
        /// @param callback Callback to invoke when files change
        void Start(FileChangeCallback callback);

        /// Stop watching for changes
        void Stop();

        /// Process pending file change events on the main thread
        /// Call this periodically to handle callbacks safely
        void ProcessEvents();

        /// Get the watch path
        [[nodiscard]] const std::filesystem::path& GetWatchPath() const { return m_config.watchPath; }

        /// Get the configuration
        [[nodiscard]] const FileWatcherConfig& GetConfig() const { return m_config; }

    private:
        /// Check if a file extension matches the filter
        [[nodiscard]] bool MatchesExtensionFilter(const std::filesystem::path& path) const;

        /// Worker thread function
        void WatcherThread();

        /// Queue an event for processing on the main thread
        void QueueEvent(FileChangeEvent event);

#if FORGE_PLATFORM_WINDOWS
        /// Windows-specific directory handle
        void* m_directoryHandle = nullptr;
#endif

        FileWatcherConfig m_config;
        FileChangeCallback m_callback;

        // Threading
        std::thread m_watcherThread;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_shouldStop{false};

        // Event queue (for main thread processing)
        Vector<FileChangeEvent> m_eventQueue;
        std::mutex m_eventMutex;
    };

    // ========================================================================
    // Multi-Directory File Watcher
    // ========================================================================

    /// Watches multiple directories with a single callback
    class MultiFileWatcher : public NonCopyable
    {
    public:
        MultiFileWatcher() = default;
        ~MultiFileWatcher();

        /// Add a directory to watch
        /// @param config Configuration for the directory
        /// @return Index of the watcher, or -1 if failed
        i32 AddWatch(const FileWatcherConfig& config);

        /// Remove a directory watch
        /// @param index Index of the watcher to remove
        void RemoveWatch(i32 index);

        /// Remove all watches
        void RemoveAllWatches();

        /// Start watching all directories
        /// @param callback Callback for file changes
        void Start(FileChangeCallback callback);

        /// Stop watching all directories
        void Stop();

        /// Process pending events from all watchers
        void ProcessEvents();

        /// Get number of active watchers
        [[nodiscard]] usize GetWatchCount() const { return m_watchers.size(); }

    private:
        Vector<UniquePtr<FileWatcher>> m_watchers;
        FileChangeCallback m_callback;
    };

} // namespace Forge::Platform
