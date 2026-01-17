#pragma once

// Save/Load Manager for Evolution Simulator
// Handles game state persistence with auto-save and save slot management

#include "Serializer.h"
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <filesystem>
#include <cstdlib>

namespace Forge {

// ============================================================================
// Save Slot Information
// ============================================================================

struct SaveSlotInfo {
    std::string filename;
    std::string displayName;
    uint64_t timestamp = 0;
    uint32_t creatureCount = 0;
    uint32_t generation = 0;
    float simulationTime = 0.0f;
    bool valid = false;

    // Convert timestamp to readable string
    std::string getTimestampString() const {
        if (timestamp == 0) return "Unknown";
        auto time = std::chrono::system_clock::from_time_t(static_cast<time_t>(timestamp));
        auto timeT = std::chrono::system_clock::to_time_t(time);
        char buffer[64];
        struct tm timeInfo;
#ifdef _WIN32
        localtime_s(&timeInfo, &timeT);
#else
        localtime_r(&timeT, &timeInfo);
#endif
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
        return std::string(buffer);
    }
};

// ============================================================================
// Save Result
// ============================================================================

enum class SaveResult {
    Success,
    FailedToOpen,
    WriteError,
    InvalidData
};

enum class LoadResult {
    Success,
    FileNotFound,
    InvalidFormat,
    VersionMismatch,
    ReadError,
    CorruptedData
};

// ============================================================================
// Save Manager
// ============================================================================

class SaveManager {
public:
    SaveManager();
    ~SaveManager() = default;

    // Save directory management
    void setSaveDirectory(const std::string& dir) { m_saveDirectory = dir; }
    const std::string& getSaveDirectory() const { return m_saveDirectory; }
    void ensureSaveDirectory();

    // Save operations
    SaveResult saveGame(const std::string& filename, const SaveFileHeader& header,
                        const WorldSaveData& world,
                        const std::vector<CreatureSaveData>& creatures,
                        const std::vector<FoodSaveData>& food);

    SaveResult quickSave(const SaveFileHeader& header,
                         const WorldSaveData& world,
                         const std::vector<CreatureSaveData>& creatures,
                         const std::vector<FoodSaveData>& food);

    // Load operations
    LoadResult loadGame(const std::string& filename, SaveFileHeader& header,
                        WorldSaveData& world,
                        std::vector<CreatureSaveData>& creatures,
                        std::vector<FoodSaveData>& food);

    LoadResult quickLoad(SaveFileHeader& header,
                         WorldSaveData& world,
                         std::vector<CreatureSaveData>& creatures,
                         std::vector<FoodSaveData>& food);

    // Save slot management
    std::vector<SaveSlotInfo> listSaveSlots();
    bool deleteSave(const std::string& filename);
    bool renameSave(const std::string& oldName, const std::string& newName);
    SaveSlotInfo getSaveInfo(const std::string& filename);

    // Auto-save functionality
    void enableAutoSave(float intervalSeconds);
    void disableAutoSave();
    bool isAutoSaveEnabled() const { return m_autoSaveEnabled; }
    float getAutoSaveInterval() const { return m_autoSaveInterval; }

    // Call this every frame to check for auto-save
    // Returns true if an auto-save was triggered
    bool update(float dt);

    // Callback for when auto-save should occur
    using AutoSaveCallback = std::function<void(const std::string& filename)>;
    void setAutoSaveCallback(AutoSaveCallback callback) { m_autoSaveCallback = callback; }

    // Get last error message
    const std::string& getLastError() const { return m_lastError; }

    // Static helper to get default save path
    static std::string getDefaultSaveDirectory();

private:
    std::string m_saveDirectory;
    std::string m_lastError;

    // Auto-save state
    bool m_autoSaveEnabled = false;
    float m_autoSaveInterval = 300.0f;  // 5 minutes default
    float m_timeSinceLastSave = 0.0f;
    int m_autoSaveSlot = 0;
    static constexpr int MAX_AUTO_SAVES = 3;

    AutoSaveCallback m_autoSaveCallback;

    std::string getFullPath(const std::string& filename) const;
    std::string getQuickSavePath() const;
    std::string getAutoSavePath() const;
};

// ============================================================================
// Implementation
// ============================================================================

inline SaveManager::SaveManager() {
    m_saveDirectory = getDefaultSaveDirectory();
}

inline void SaveManager::ensureSaveDirectory() {
    try {
        std::filesystem::create_directories(m_saveDirectory);
    } catch (const std::exception& e) {
        m_lastError = "Failed to create save directory: " + std::string(e.what());
    }
}

inline std::string SaveManager::getDefaultSaveDirectory() {
#ifdef _WIN32
    char* appdata = nullptr;
    size_t appdataLen = 0;
    if (_dupenv_s(&appdata, &appdataLen, "APPDATA") == 0 && appdata) {
        std::string path(appdata);
        std::free(appdata);
        return path + "\\OrganismEvolution\\saves";
    }
    if (appdata) {
        std::free(appdata);
    }
    return ".\\saves";
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/OrganismEvolution/saves";
    }
    return "./saves";
#endif
}

inline std::string SaveManager::getFullPath(const std::string& filename) const {
    if (filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos) {
        // Already a path
        return filename;
    }
    return m_saveDirectory + "/" + filename;
}

inline std::string SaveManager::getQuickSavePath() const {
    return getFullPath("quicksave.evos");
}

inline std::string SaveManager::getAutoSavePath() const {
    return getFullPath("autosave_" + std::to_string(m_autoSaveSlot) + ".evos");
}

inline SaveResult SaveManager::saveGame(const std::string& filename,
                                        const SaveFileHeader& header,
                                        const WorldSaveData& world,
                                        const std::vector<CreatureSaveData>& creatures,
                                        const std::vector<FoodSaveData>& food) {
    ensureSaveDirectory();

    BinaryWriter writer;
    std::string fullPath = getFullPath(filename);

    if (!writer.open(fullPath)) {
        m_lastError = "Failed to open file for writing: " + fullPath;
        return SaveResult::FailedToOpen;
    }

    try {
        // Write header
        header.write(writer);

        // Write world state
        writer.write(SaveConstants::CHUNK_WORLD);
        world.write(writer);

        // Write creatures
        writer.write(SaveConstants::CHUNK_CREATURES);
        writer.write(static_cast<uint32_t>(creatures.size()));
        for (const auto& creature : creatures) {
            creature.write(writer);
        }

        // Write food
        writer.write(SaveConstants::CHUNK_FOOD);
        writer.write(static_cast<uint32_t>(food.size()));
        for (const auto& f : food) {
            f.write(writer);
        }

        writer.close();
        return SaveResult::Success;

    } catch (const std::exception& e) {
        m_lastError = "Write error: " + std::string(e.what());
        return SaveResult::WriteError;
    }
}

inline SaveResult SaveManager::quickSave(const SaveFileHeader& header,
                                         const WorldSaveData& world,
                                         const std::vector<CreatureSaveData>& creatures,
                                         const std::vector<FoodSaveData>& food) {
    return saveGame(getQuickSavePath(), header, world, creatures, food);
}

inline LoadResult SaveManager::loadGame(const std::string& filename,
                                        SaveFileHeader& header,
                                        WorldSaveData& world,
                                        std::vector<CreatureSaveData>& creatures,
                                        std::vector<FoodSaveData>& food) {
    BinaryReader reader;
    std::string fullPath = getFullPath(filename);

    if (!reader.open(fullPath)) {
        m_lastError = "Failed to open file: " + fullPath;
        return LoadResult::FileNotFound;
    }

    try {
        // Read and validate header
        if (!header.read(reader)) {
            m_lastError = "Invalid save file format";
            return LoadResult::InvalidFormat;
        }

        if (header.version > SaveConstants::CURRENT_VERSION) {
            m_lastError = "Save file version " + std::to_string(header.version) +
                          " is newer than supported version " +
                          std::to_string(SaveConstants::CURRENT_VERSION);
            return LoadResult::VersionMismatch;
        }

        if (header.version < SaveConstants::MIN_SUPPORTED_VERSION) {
            m_lastError = "Save file version " + std::to_string(header.version) +
                          " is older than minimum supported version " +
                          std::to_string(SaveConstants::MIN_SUPPORTED_VERSION);
            return LoadResult::VersionMismatch;
        }

        // Read world state (pass version for backward compatibility)
        uint32_t chunkId = reader.read<uint32_t>();
        if (chunkId != SaveConstants::CHUNK_WORLD) {
            m_lastError = "Expected world chunk, got " + std::to_string(chunkId);
            return LoadResult::CorruptedData;
        }
        world.read(reader, header.version);

        // Read creatures with bounds checking
        chunkId = reader.read<uint32_t>();
        if (chunkId != SaveConstants::CHUNK_CREATURES) {
            m_lastError = "Expected creatures chunk";
            return LoadResult::CorruptedData;
        }
        uint32_t creatureCount = reader.read<uint32_t>();
        if (creatureCount > SaveConstants::MAX_CREATURES) {
            m_lastError = "Creature count " + std::to_string(creatureCount) +
                          " exceeds maximum " + std::to_string(SaveConstants::MAX_CREATURES);
            return LoadResult::CorruptedData;
        }
        creatures.resize(creatureCount);
        for (uint32_t i = 0; i < creatureCount; ++i) {
            creatures[i].read(reader);
        }

        // Read food with bounds checking
        chunkId = reader.read<uint32_t>();
        if (chunkId != SaveConstants::CHUNK_FOOD) {
            m_lastError = "Expected food chunk";
            return LoadResult::CorruptedData;
        }
        uint32_t foodCount = reader.read<uint32_t>();
        if (foodCount > SaveConstants::MAX_FOOD) {
            m_lastError = "Food count " + std::to_string(foodCount) +
                          " exceeds maximum " + std::to_string(SaveConstants::MAX_FOOD);
            return LoadResult::CorruptedData;
        }
        food.resize(foodCount);
        for (uint32_t i = 0; i < foodCount; ++i) {
            food[i].read(reader);
        }

        reader.close();
        return LoadResult::Success;

    } catch (const std::exception& e) {
        m_lastError = "Read error: " + std::string(e.what());
        return LoadResult::ReadError;
    }
}

inline LoadResult SaveManager::quickLoad(SaveFileHeader& header,
                                         WorldSaveData& world,
                                         std::vector<CreatureSaveData>& creatures,
                                         std::vector<FoodSaveData>& food) {
    return loadGame(getQuickSavePath(), header, world, creatures, food);
}

inline std::vector<SaveSlotInfo> SaveManager::listSaveSlots() {
    std::vector<SaveSlotInfo> slots;

    try {
        if (!std::filesystem::exists(m_saveDirectory)) {
            return slots;
        }

        for (const auto& entry : std::filesystem::directory_iterator(m_saveDirectory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".evos") {
                    SaveSlotInfo info = getSaveInfo(entry.path().string());
                    if (info.valid) {
                        slots.push_back(info);
                    }
                }
            }
        }

        // Sort by timestamp (newest first)
        std::sort(slots.begin(), slots.end(),
                  [](const SaveSlotInfo& a, const SaveSlotInfo& b) {
                      return a.timestamp > b.timestamp;
                  });

    } catch (const std::exception& e) {
        m_lastError = "Failed to list save slots: " + std::string(e.what());
    }

    return slots;
}

inline SaveSlotInfo SaveManager::getSaveInfo(const std::string& filename) {
    SaveSlotInfo info;
    info.filename = filename;

    BinaryReader reader;
    if (!reader.open(filename)) {
        return info;
    }

    try {
        SaveFileHeader header;
        if (header.read(reader)) {
            info.valid = true;
            info.timestamp = header.timestamp;
            info.creatureCount = header.creatureCount;
            info.generation = header.generation;
            info.simulationTime = header.simulationTime;

            // Extract display name from filename
            std::filesystem::path path(filename);
            info.displayName = path.stem().string();
        }
    } catch (...) {
        // Invalid file
    }

    return info;
}

inline bool SaveManager::deleteSave(const std::string& filename) {
    try {
        std::string fullPath = getFullPath(filename);
        return std::filesystem::remove(fullPath);
    } catch (const std::exception& e) {
        m_lastError = "Failed to delete save: " + std::string(e.what());
        return false;
    }
}

inline bool SaveManager::renameSave(const std::string& oldName, const std::string& newName) {
    try {
        std::string oldPath = getFullPath(oldName);
        std::string newPath = getFullPath(newName);
        std::filesystem::rename(oldPath, newPath);
        return true;
    } catch (const std::exception& e) {
        m_lastError = "Failed to rename save: " + std::string(e.what());
        return false;
    }
}

inline void SaveManager::enableAutoSave(float intervalSeconds) {
    m_autoSaveEnabled = true;
    m_autoSaveInterval = intervalSeconds;
    m_timeSinceLastSave = 0.0f;
}

inline void SaveManager::disableAutoSave() {
    m_autoSaveEnabled = false;
}

inline bool SaveManager::update(float dt) {
    if (!m_autoSaveEnabled) {
        return false;
    }

    m_timeSinceLastSave += dt;

    if (m_timeSinceLastSave >= m_autoSaveInterval) {
        m_timeSinceLastSave = 0.0f;

        if (m_autoSaveCallback) {
            std::string savePath = getAutoSavePath();
            m_autoSaveCallback(savePath);

            // Rotate auto-save slots
            m_autoSaveSlot = (m_autoSaveSlot + 1) % MAX_AUTO_SAVES;
            return true;
        }
    }

    return false;
}

} // namespace Forge
