#pragma once

// Binary Serialization Utilities for Evolution Simulator
// Provides type-safe binary read/write operations with versioning support

#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <type_traits>
#include <stdexcept>

namespace Forge {

// Forward declarations for Math types
namespace Math {
    struct Vec3;
}

// ============================================================================
// Binary Writer - Writes data to binary files
// ============================================================================

class BinaryWriter {
public:
    BinaryWriter() = default;
    ~BinaryWriter() { close(); }

    // Non-copyable
    BinaryWriter(const BinaryWriter&) = delete;
    BinaryWriter& operator=(const BinaryWriter&) = delete;

    // Movable
    BinaryWriter(BinaryWriter&& other) noexcept : m_file(std::move(other.m_file)) {}
    BinaryWriter& operator=(BinaryWriter&& other) noexcept {
        if (this != &other) {
            close();
            m_file = std::move(other.m_file);
        }
        return *this;
    }

    bool open(const std::string& filename) {
        m_file.open(filename, std::ios::binary | std::ios::out);
        return m_file.is_open();
    }

    void close() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    bool isOpen() const { return m_file.is_open(); }

    // Write raw bytes
    void writeRaw(const void* data, size_t size) {
        m_file.write(static_cast<const char*>(data), size);
    }

    // Write primitive types
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value>::type
    write(const T& value) {
        writeRaw(&value, sizeof(T));
    }

    // Write string (length-prefixed)
    void writeString(const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.size());
        write(len);
        if (len > 0) {
            writeRaw(str.data(), len);
        }
    }

    // Write vector of primitives
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value>::type
    writeVector(const std::vector<T>& vec) {
        uint32_t count = static_cast<uint32_t>(vec.size());
        write(count);
        if (count > 0) {
            writeRaw(vec.data(), count * sizeof(T));
        }
    }

    // Write Vec3 (3 floats)
    void writeVec3(float x, float y, float z) {
        write(x);
        write(y);
        write(z);
    }

    // Write bool as uint8_t
    void writeBool(bool value) {
        uint8_t b = value ? 1 : 0;
        write(b);
    }

    // Write enum as underlying type
    template<typename E>
    typename std::enable_if<std::is_enum<E>::value>::type
    writeEnum(E value) {
        write(static_cast<typename std::underlying_type<E>::type>(value));
    }

    // Get current write position
    std::streampos getPosition() {
        return m_file.tellp();
    }

    // Seek to position
    void seek(std::streampos pos) {
        m_file.seekp(pos);
    }

private:
    std::ofstream m_file;
};

// ============================================================================
// Binary Reader - Reads data from binary files
// ============================================================================

class BinaryReader {
public:
    BinaryReader() = default;
    ~BinaryReader() { close(); }

    // Non-copyable
    BinaryReader(const BinaryReader&) = delete;
    BinaryReader& operator=(const BinaryReader&) = delete;

    // Movable
    BinaryReader(BinaryReader&& other) noexcept : m_file(std::move(other.m_file)) {}
    BinaryReader& operator=(BinaryReader&& other) noexcept {
        if (this != &other) {
            close();
            m_file = std::move(other.m_file);
        }
        return *this;
    }

    bool open(const std::string& filename) {
        m_file.open(filename, std::ios::binary | std::ios::in);
        return m_file.is_open();
    }

    void close() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    bool isOpen() const { return m_file.is_open(); }

    bool eof() const { return m_file.eof(); }

    bool good() const { return m_file.good(); }

    // Read raw bytes
    void readRaw(void* data, size_t size) {
        m_file.read(static_cast<char*>(data), size);
    }

    // Read primitive types
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, T>::type
    read() {
        T value;
        readRaw(&value, sizeof(T));
        return value;
    }

    // Read string (length-prefixed) with bounds checking
    std::string readString(uint32_t maxLength = 16 * 1024 * 1024) {
        uint32_t len = read<uint32_t>();
        if (len == 0) return "";
        if (len > maxLength) {
            throw std::runtime_error("String length " + std::to_string(len) +
                                     " exceeds maximum allowed " + std::to_string(maxLength));
        }
        std::string str(len, '\0');
        readRaw(&str[0], len);
        return str;
    }

    // Read vector of primitives with bounds checking
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, std::vector<T>>::type
    readVector(uint32_t maxElements = 10 * 1000 * 1000) {
        uint32_t count = read<uint32_t>();
        if (count > maxElements) {
            throw std::runtime_error("Vector element count " + std::to_string(count) +
                                     " exceeds maximum allowed " + std::to_string(maxElements));
        }
        std::vector<T> vec(count);
        if (count > 0) {
            readRaw(vec.data(), count * sizeof(T));
        }
        return vec;
    }

    // Read Vec3 (returns tuple of 3 floats)
    void readVec3(float& x, float& y, float& z) {
        x = read<float>();
        y = read<float>();
        z = read<float>();
    }

    // Read bool from uint8_t
    bool readBool() {
        return read<uint8_t>() != 0;
    }

    // Read enum from underlying type
    template<typename E>
    typename std::enable_if<std::is_enum<E>::value, E>::type
    readEnum() {
        auto value = read<typename std::underlying_type<E>::type>();
        return static_cast<E>(value);
    }

    // Get current read position
    std::streampos getPosition() {
        return m_file.tellg();
    }

    // Seek to position
    void seek(std::streampos pos) {
        m_file.seekg(pos);
    }

    // Get file size
    std::streampos getFileSize() {
        auto current = m_file.tellg();
        m_file.seekg(0, std::ios::end);
        auto size = m_file.tellg();
        m_file.seekg(current);
        return size;
    }

private:
    std::ifstream m_file;
};

// ============================================================================
// Save File Constants
// ============================================================================

namespace SaveConstants {
    // Magic number "EVOS" = Evolution Save
    constexpr uint32_t MAGIC_NUMBER = 0x534F5645;  // "EVOS" in little-endian

    // Current save format version
    // Version 1: Initial format
    // Version 2: Added RNG state as string, maxGeneration, nextCreatureId
    constexpr uint32_t CURRENT_VERSION = 2;
    constexpr uint32_t MIN_SUPPORTED_VERSION = 1;

    // Security limits for deserialization
    constexpr uint32_t MAX_STRING_LENGTH = 16 * 1024 * 1024;       // 16 MB max string
    constexpr uint32_t MAX_VECTOR_ELEMENTS = 10 * 1000 * 1000;     // 10 million elements max
    constexpr uint32_t MAX_CREATURES = 100000;                      // 100k creatures max
    constexpr uint32_t MAX_FOOD = 1000000;                          // 1 million food max
    constexpr uint32_t MAX_NEURAL_WEIGHTS = 100000;                 // 100k weights per creature

    // Chunk identifiers for extensibility
    constexpr uint32_t CHUNK_HEADER     = 0x48445200;  // "HDR\0"
    constexpr uint32_t CHUNK_WORLD      = 0x574C4400;  // "WLD\0"
    constexpr uint32_t CHUNK_CREATURES  = 0x43525400;  // "CRT\0"
    constexpr uint32_t CHUNK_FOOD       = 0x464F4F44;  // "FOOD"
    constexpr uint32_t CHUNK_TERRAIN    = 0x54455252;  // "TERR"
    constexpr uint32_t CHUNK_REPLAY     = 0x52504C59;  // "RPLY"
}

// ============================================================================
// Save File Header
// ============================================================================

struct SaveFileHeader {
    uint32_t magic = SaveConstants::MAGIC_NUMBER;
    uint32_t version = SaveConstants::CURRENT_VERSION;
    uint64_t timestamp = 0;          // Unix timestamp when saved
    uint32_t creatureCount = 0;
    uint32_t foodCount = 0;
    uint32_t generation = 0;
    float simulationTime = 0.0f;
    uint32_t terrainSeed = 0;        // For terrain regeneration
    uint32_t flags = 0;              // Reserved for future use

    void write(BinaryWriter& writer) const {
        writer.write(magic);
        writer.write(version);
        writer.write(timestamp);
        writer.write(creatureCount);
        writer.write(foodCount);
        writer.write(generation);
        writer.write(simulationTime);
        writer.write(terrainSeed);
        writer.write(flags);
    }

    bool read(BinaryReader& reader) {
        magic = reader.read<uint32_t>();
        if (magic != SaveConstants::MAGIC_NUMBER) {
            return false;
        }
        version = reader.read<uint32_t>();
        timestamp = reader.read<uint64_t>();
        creatureCount = reader.read<uint32_t>();
        foodCount = reader.read<uint32_t>();
        generation = reader.read<uint32_t>();
        simulationTime = reader.read<float>();
        terrainSeed = reader.read<uint32_t>();
        flags = reader.read<uint32_t>();
        return true;
    }
};

// ============================================================================
// Creature Save Data
// ============================================================================

struct CreatureSaveData {
    uint32_t id = 0;
    uint8_t type = 0;  // CreatureType as uint8_t

    // Position and physics
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float velX = 0.0f, velY = 0.0f, velZ = 0.0f;
    float rotation = 0.0f;

    // State
    float health = 100.0f;
    float energy = 100.0f;
    float age = 0.0f;
    int32_t generation = 1;

    // Behavior tracking
    float foodEaten = 0.0f;
    float distanceTraveled = 0.0f;
    int32_t successfulHunts = 0;
    int32_t escapes = 0;
    float wanderAngle = 0.0f;
    float animPhase = 0.0f;

    // Genome data
    float genomeSize = 1.0f;
    float genomeSpeed = 10.0f;
    float genomeVision = 30.0f;
    float genomeEfficiency = 1.0f;
    float genomeColorR = 0.5f;
    float genomeColorG = 0.5f;
    float genomeColorB = 0.5f;
    float genomeMutationRate = 0.1f;

    // Neural network weights
    std::vector<float> weightsIH;
    std::vector<float> weightsHO;
    std::vector<float> biasH;
    std::vector<float> biasO;

    void write(BinaryWriter& writer) const {
        writer.write(id);
        writer.write(type);

        writer.write(posX);
        writer.write(posY);
        writer.write(posZ);
        writer.write(velX);
        writer.write(velY);
        writer.write(velZ);
        writer.write(rotation);

        writer.write(health);
        writer.write(energy);
        writer.write(age);
        writer.write(generation);

        writer.write(foodEaten);
        writer.write(distanceTraveled);
        writer.write(successfulHunts);
        writer.write(escapes);
        writer.write(wanderAngle);
        writer.write(animPhase);

        writer.write(genomeSize);
        writer.write(genomeSpeed);
        writer.write(genomeVision);
        writer.write(genomeEfficiency);
        writer.write(genomeColorR);
        writer.write(genomeColorG);
        writer.write(genomeColorB);
        writer.write(genomeMutationRate);

        writer.writeVector(weightsIH);
        writer.writeVector(weightsHO);
        writer.writeVector(biasH);
        writer.writeVector(biasO);
    }

    void read(BinaryReader& reader) {
        id = reader.read<uint32_t>();
        type = reader.read<uint8_t>();

        posX = reader.read<float>();
        posY = reader.read<float>();
        posZ = reader.read<float>();
        velX = reader.read<float>();
        velY = reader.read<float>();
        velZ = reader.read<float>();
        rotation = reader.read<float>();

        health = reader.read<float>();
        energy = reader.read<float>();
        age = reader.read<float>();
        generation = reader.read<int32_t>();

        foodEaten = reader.read<float>();
        distanceTraveled = reader.read<float>();
        successfulHunts = reader.read<int32_t>();
        escapes = reader.read<int32_t>();
        wanderAngle = reader.read<float>();
        animPhase = reader.read<float>();

        genomeSize = reader.read<float>();
        genomeSpeed = reader.read<float>();
        genomeVision = reader.read<float>();
        genomeEfficiency = reader.read<float>();
        genomeColorR = reader.read<float>();
        genomeColorG = reader.read<float>();
        genomeColorB = reader.read<float>();
        genomeMutationRate = reader.read<float>();

        // Read neural network weights with bounds checking (max 100k weights per array)
        weightsIH = reader.readVector<float>(SaveConstants::MAX_NEURAL_WEIGHTS);
        weightsHO = reader.readVector<float>(SaveConstants::MAX_NEURAL_WEIGHTS);
        biasH = reader.readVector<float>(SaveConstants::MAX_NEURAL_WEIGHTS);
        biasO = reader.readVector<float>(SaveConstants::MAX_NEURAL_WEIGHTS);
    }
};

// ============================================================================
// Food Save Data
// ============================================================================

struct FoodSaveData {
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float energy = 20.0f;
    float respawnTimer = 0.0f;
    bool active = true;

    void write(BinaryWriter& writer) const {
        writer.write(posX);
        writer.write(posY);
        writer.write(posZ);
        writer.write(energy);
        writer.write(respawnTimer);
        writer.writeBool(active);
    }

    void read(BinaryReader& reader) {
        posX = reader.read<float>();
        posY = reader.read<float>();
        posZ = reader.read<float>();
        energy = reader.read<float>();
        respawnTimer = reader.read<float>();
        active = reader.readBool();
    }
};

// ============================================================================
// World State Save Data
// ============================================================================

struct WorldSaveData {
    uint32_t terrainSeed = 12345;
    float dayTime = 0.5f;           // 0-1 representing time of day
    float dayDuration = 120.0f;     // Seconds per day cycle
    std::string rngState;           // Full RNG state for deterministic replay (v2+)
    uint32_t maxGeneration = 1;     // Highest generation in simulation (v2+)
    uint32_t nextCreatureId = 0;    // Next creature ID to assign (v2+)

    void write(BinaryWriter& writer) const {
        writer.write(terrainSeed);
        writer.write(dayTime);
        writer.write(dayDuration);
        writer.writeString(rngState);
        writer.write(maxGeneration);
        writer.write(nextCreatureId);
    }

    // Version-aware read
    void read(BinaryReader& reader, uint32_t version = SaveConstants::CURRENT_VERSION) {
        terrainSeed = reader.read<uint32_t>();
        dayTime = reader.read<float>();
        dayDuration = reader.read<float>();

        if (version >= 2) {
            // V2+ format: RNG state as string, maxGeneration, nextCreatureId
            rngState = reader.readString(SaveConstants::MAX_STRING_LENGTH);
            maxGeneration = reader.read<uint32_t>();
            nextCreatureId = reader.read<uint32_t>();
        } else {
            // V1 format: had uint32_t rngState (placeholder, always 0)
            uint32_t legacyRngState = reader.read<uint32_t>();
            (void)legacyRngState;  // Discard - was always 0, not useful
            rngState.clear();
            maxGeneration = 1;
            nextCreatureId = 0;
        }
    }
};

} // namespace Forge
