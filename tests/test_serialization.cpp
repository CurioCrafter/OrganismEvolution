// test_serialization.cpp - Unit tests for save/load serialization
// Tests data structure round-trip integrity

#include "core/Serializer.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>
#include <sstream>
#include <cstdio>

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// Test SaveFileHeader serialization
void testHeaderSerialization() {
    std::cout << "Testing SaveFileHeader serialization..." << std::endl;

    Forge::SaveFileHeader original;
    original.magic = Forge::SaveConstants::MAGIC_NUMBER;
    original.version = Forge::SaveConstants::CURRENT_VERSION;
    original.timestamp = 1234567890ULL;
    original.creatureCount = 500;
    original.foodCount = 1000;
    original.generation = 42;
    original.simulationTime = 3600.5f;
    original.terrainSeed = 98765;
    original.flags = 0;

    // Create temp file
    const char* tempFile = "test_header_temp.bin";

    // Write
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));
        original.write(writer);
        writer.close();
    }

    // Read back
    Forge::SaveFileHeader loaded;
    {
        Forge::BinaryReader reader;
        assert(reader.open(tempFile));
        assert(loaded.read(reader));
        reader.close();
    }

    // Verify
    assert(loaded.magic == original.magic);
    assert(loaded.version == original.version);
    assert(loaded.timestamp == original.timestamp);
    assert(loaded.creatureCount == original.creatureCount);
    assert(loaded.foodCount == original.foodCount);
    assert(loaded.generation == original.generation);
    assert(approxEqual(loaded.simulationTime, original.simulationTime));
    assert(loaded.terrainSeed == original.terrainSeed);
    assert(loaded.flags == original.flags);

    // Cleanup
    std::remove(tempFile);

    std::cout << "  SaveFileHeader serialization test passed!" << std::endl;
}

// Test CreatureSaveData serialization
void testCreatureSaveData() {
    std::cout << "Testing CreatureSaveData serialization..." << std::endl;

    Forge::CreatureSaveData original;
    original.id = 12345;
    original.type = 3;  // CreatureType
    original.posX = 50.5f;
    original.posY = 10.0f;
    original.posZ = 75.25f;
    original.velX = 1.5f;
    original.velY = 0.0f;
    original.velZ = -2.5f;
    original.rotation = 1.57f;
    original.health = 85.5f;
    original.energy = 60.0f;
    original.age = 120.5f;
    original.generation = 7;
    original.foodEaten = 25.5f;
    original.distanceTraveled = 1500.0f;
    original.successfulHunts = 10;
    original.escapes = 5;
    original.wanderAngle = 0.75f;
    original.animPhase = 0.5f;
    original.genomeSize = 1.5f;
    original.genomeSpeed = 12.0f;
    original.genomeVision = 35.0f;
    original.genomeEfficiency = 0.85f;
    original.genomeColorR = 0.2f;
    original.genomeColorG = 0.6f;
    original.genomeColorB = 0.4f;
    original.genomeMutationRate = 0.15f;

    // Neural weights
    original.weightsIH = {0.1f, 0.2f, 0.3f, -0.1f, -0.2f};
    original.weightsHO = {0.5f, 0.6f, -0.5f};
    original.biasH = {0.01f, 0.02f};
    original.biasO = {0.001f};

    const char* tempFile = "test_creature_temp.bin";

    // Write
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));
        original.write(writer);
        writer.close();
    }

    // Read back
    Forge::CreatureSaveData loaded;
    {
        Forge::BinaryReader reader;
        assert(reader.open(tempFile));
        loaded.read(reader);
        reader.close();
    }

    // Verify all fields
    assert(loaded.id == original.id);
    assert(loaded.type == original.type);
    assert(approxEqual(loaded.posX, original.posX));
    assert(approxEqual(loaded.posY, original.posY));
    assert(approxEqual(loaded.posZ, original.posZ));
    assert(approxEqual(loaded.velX, original.velX));
    assert(approxEqual(loaded.velY, original.velY));
    assert(approxEqual(loaded.velZ, original.velZ));
    assert(approxEqual(loaded.rotation, original.rotation));
    assert(approxEqual(loaded.health, original.health));
    assert(approxEqual(loaded.energy, original.energy));
    assert(approxEqual(loaded.age, original.age));
    assert(loaded.generation == original.generation);
    assert(approxEqual(loaded.foodEaten, original.foodEaten));
    assert(approxEqual(loaded.distanceTraveled, original.distanceTraveled));
    assert(loaded.successfulHunts == original.successfulHunts);
    assert(loaded.escapes == original.escapes);
    assert(approxEqual(loaded.genomeSize, original.genomeSize));
    assert(approxEqual(loaded.genomeSpeed, original.genomeSpeed));
    assert(approxEqual(loaded.genomeVision, original.genomeVision));
    assert(approxEqual(loaded.genomeEfficiency, original.genomeEfficiency));
    assert(approxEqual(loaded.genomeColorR, original.genomeColorR));
    assert(approxEqual(loaded.genomeColorG, original.genomeColorG));
    assert(approxEqual(loaded.genomeColorB, original.genomeColorB));
    assert(approxEqual(loaded.genomeMutationRate, original.genomeMutationRate));

    // Verify neural weights
    assert(loaded.weightsIH.size() == original.weightsIH.size());
    for (size_t i = 0; i < original.weightsIH.size(); i++) {
        assert(approxEqual(loaded.weightsIH[i], original.weightsIH[i]));
    }
    assert(loaded.weightsHO.size() == original.weightsHO.size());
    assert(loaded.biasH.size() == original.biasH.size());
    assert(loaded.biasO.size() == original.biasO.size());

    std::remove(tempFile);

    std::cout << "  CreatureSaveData serialization test passed!" << std::endl;
}

// Test FoodSaveData serialization
void testFoodSaveData() {
    std::cout << "Testing FoodSaveData serialization..." << std::endl;

    Forge::FoodSaveData original;
    original.posX = 25.5f;
    original.posY = 0.0f;
    original.posZ = 75.25f;
    original.energy = 30.0f;
    original.respawnTimer = 5.5f;
    original.active = true;

    const char* tempFile = "test_food_temp.bin";

    // Write
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));
        original.write(writer);
        writer.close();
    }

    // Read back
    Forge::FoodSaveData loaded;
    {
        Forge::BinaryReader reader;
        assert(reader.open(tempFile));
        loaded.read(reader);
        reader.close();
    }

    // Verify
    assert(approxEqual(loaded.posX, original.posX));
    assert(approxEqual(loaded.posY, original.posY));
    assert(approxEqual(loaded.posZ, original.posZ));
    assert(approxEqual(loaded.energy, original.energy));
    assert(approxEqual(loaded.respawnTimer, original.respawnTimer));
    assert(loaded.active == original.active);

    std::remove(tempFile);

    std::cout << "  FoodSaveData serialization test passed!" << std::endl;
}

// Test WorldSaveData serialization
void testWorldSaveData() {
    std::cout << "Testing WorldSaveData serialization..." << std::endl;

    Forge::WorldSaveData original;
    original.terrainSeed = 54321;
    original.dayTime = 0.75f;
    original.dayDuration = 180.0f;
    original.rngState = 987654321;

    const char* tempFile = "test_world_temp.bin";

    // Write
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));
        original.write(writer);
        writer.close();
    }

    // Read back
    Forge::WorldSaveData loaded;
    {
        Forge::BinaryReader reader;
        assert(reader.open(tempFile));
        loaded.read(reader);
        reader.close();
    }

    // Verify
    assert(loaded.terrainSeed == original.terrainSeed);
    assert(approxEqual(loaded.dayTime, original.dayTime));
    assert(approxEqual(loaded.dayDuration, original.dayDuration));
    assert(loaded.rngState == original.rngState);

    std::remove(tempFile);

    std::cout << "  WorldSaveData serialization test passed!" << std::endl;
}

// Test BinaryWriter/Reader primitives
void testBinaryPrimitives() {
    std::cout << "Testing binary primitives..." << std::endl;

    const char* tempFile = "test_primitives_temp.bin";

    // Write various types
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));

        writer.write<uint8_t>(255);
        writer.write<int16_t>(-1234);
        writer.write<uint32_t>(4000000000);
        writer.write<int64_t>(-9000000000000LL);
        writer.write<float>(3.14159f);
        writer.write<double>(2.718281828);
        writer.writeBool(true);
        writer.writeBool(false);
        writer.writeString("Hello, World!");
        writer.writeString("");  // Empty string

        std::vector<float> floats = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        writer.writeVector(floats);

        writer.close();
    }

    // Read back
    {
        Forge::BinaryReader reader;
        assert(reader.open(tempFile));

        assert(reader.read<uint8_t>() == 255);
        assert(reader.read<int16_t>() == -1234);
        assert(reader.read<uint32_t>() == 4000000000);
        assert(reader.read<int64_t>() == -9000000000000LL);
        assert(approxEqual(reader.read<float>(), 3.14159f));
        assert(std::abs(reader.read<double>() - 2.718281828) < 0.0001);
        assert(reader.readBool() == true);
        assert(reader.readBool() == false);
        assert(reader.readString() == "Hello, World!");
        assert(reader.readString() == "");

        auto floats = reader.readVector<float>();
        assert(floats.size() == 5);
        assert(approxEqual(floats[0], 1.0f));
        assert(approxEqual(floats[4], 5.0f));

        reader.close();
    }

    std::remove(tempFile);

    std::cout << "  Binary primitives test passed!" << std::endl;
}

// Test multiple creatures round-trip
void testMultipleCreaturesRoundTrip() {
    std::cout << "Testing multiple creatures round-trip..." << std::endl;

    const int NUM_CREATURES = 100;
    std::vector<Forge::CreatureSaveData> originals;

    // Create varied creature data
    for (int i = 0; i < NUM_CREATURES; i++) {
        Forge::CreatureSaveData c;
        c.id = i;
        c.type = i % 5;
        c.posX = (float)(i * 10);
        c.posY = 0.0f;
        c.posZ = (float)(i * 5);
        c.health = 50.0f + (float)(i % 50);
        c.energy = 30.0f + (float)(i % 70);
        c.generation = i / 10;

        // Add some neural weights
        for (int j = 0; j < 10; j++) {
            c.weightsIH.push_back((float)j * 0.1f);
        }

        originals.push_back(c);
    }

    const char* tempFile = "test_multi_creature_temp.bin";

    // Write all
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));
        writer.write<uint32_t>(NUM_CREATURES);
        for (const auto& c : originals) {
            c.write(writer);
        }
        writer.close();
    }

    // Read all back
    std::vector<Forge::CreatureSaveData> loaded;
    {
        Forge::BinaryReader reader;
        assert(reader.open(tempFile));
        uint32_t count = reader.read<uint32_t>();
        assert(count == NUM_CREATURES);
        for (uint32_t i = 0; i < count; i++) {
            Forge::CreatureSaveData c;
            c.read(reader);
            loaded.push_back(c);
        }
        reader.close();
    }

    // Verify all match
    assert(loaded.size() == originals.size());
    for (size_t i = 0; i < originals.size(); i++) {
        assert(loaded[i].id == originals[i].id);
        assert(loaded[i].type == originals[i].type);
        assert(approxEqual(loaded[i].posX, originals[i].posX));
        assert(approxEqual(loaded[i].health, originals[i].health));
        assert(loaded[i].generation == originals[i].generation);
        assert(loaded[i].weightsIH.size() == originals[i].weightsIH.size());
    }

    std::remove(tempFile);

    std::cout << "  Multiple creatures round-trip test passed!" << std::endl;
}

// Test invalid file handling
void testInvalidFileHandling() {
    std::cout << "Testing invalid file handling..." << std::endl;

    // Try to open non-existent file
    Forge::BinaryReader reader;
    assert(!reader.open("nonexistent_file_12345.bin"));

    // Try to read invalid header
    const char* tempFile = "test_invalid_temp.bin";
    {
        Forge::BinaryWriter writer;
        assert(writer.open(tempFile));
        writer.write<uint32_t>(0xDEADBEEF);  // Wrong magic number
        writer.close();
    }

    Forge::SaveFileHeader header;
    {
        Forge::BinaryReader r;
        assert(r.open(tempFile));
        assert(!header.read(r));  // Should fail due to wrong magic
        r.close();
    }

    std::remove(tempFile);

    std::cout << "  Invalid file handling test passed!" << std::endl;
}

int main() {
    std::cout << "=== Serialization Unit Tests ===" << std::endl;

    testBinaryPrimitives();
    testHeaderSerialization();
    testCreatureSaveData();
    testFoodSaveData();
    testWorldSaveData();
    testMultipleCreaturesRoundTrip();
    testInvalidFileHandling();

    std::cout << "\n=== All Serialization tests passed! ===" << std::endl;
    return 0;
}
