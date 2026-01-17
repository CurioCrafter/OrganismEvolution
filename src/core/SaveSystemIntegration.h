#pragma once

// Save System Integration for Evolution Simulator
// This header provides helper functions to integrate the save/load system
// with the SimulationWorld class in main.cpp

#include "SaveManager.h"
#include "ReplaySystem.h"
#include <chrono>
#include <cmath>

// Forward declaration for template specialization
struct SimCreature;

namespace Forge {

// ============================================================================
// Integration Helpers
// ============================================================================

// Helper to build CreatureSaveData from a Creature struct
// Note: This template assumes the Creature struct has standard members
template<typename CreatureType, typename NeuralNetType>
CreatureSaveData buildCreatureSaveData(const CreatureType& creature, const NeuralNetType& brain) {
    CreatureSaveData data;

    data.id = creature.id;
    data.type = static_cast<uint8_t>(creature.type);

    data.posX = creature.position.x;
    data.posY = creature.position.y;
    data.posZ = creature.position.z;
    data.velX = creature.velocity.x;
    data.velY = creature.velocity.y;
    data.velZ = creature.velocity.z;
    data.rotation = creature.rotation;

    data.health = creature.health;
    data.energy = creature.energy;
    data.age = creature.age;
    data.generation = creature.generation;

    data.foodEaten = creature.foodEaten;
    data.distanceTraveled = creature.distanceTraveled;
    data.successfulHunts = creature.successfulHunts;
    data.escapes = creature.escapes;
    data.wanderAngle = creature.wanderAngle;
    data.animPhase = creature.animPhase;

    // Genome
    data.genomeSize = creature.genome.size;
    data.genomeSpeed = creature.genome.speed;
    data.genomeVision = creature.genome.visionRange;
    data.genomeEfficiency = creature.genome.efficiency;
    data.genomeColorR = creature.genome.color.x;
    data.genomeColorG = creature.genome.color.y;
    data.genomeColorB = creature.genome.color.z;
    data.genomeMutationRate = creature.genome.mutationRate;

    // Neural network weights
    data.weightsIH = brain.weightsIH;
    data.weightsHO = brain.weightsHO;
    data.biasH = brain.biasH;
    data.biasO = brain.biasO;

    return data;
}

// Helper to restore Creature from CreatureSaveData
template<typename CreatureType, typename NeuralNetType, typename GenomeType>
void restoreCreatureFromSaveData(CreatureType& creature, NeuralNetType& brain,
                                  GenomeType& genome, const CreatureSaveData& data) {
    creature.id = data.id;
    creature.type = static_cast<decltype(creature.type)>(data.type);

    creature.position.x = data.posX;
    creature.position.y = data.posY;
    creature.position.z = data.posZ;
    creature.velocity.x = data.velX;
    creature.velocity.y = data.velY;
    creature.velocity.z = data.velZ;
    creature.rotation = data.rotation;

    creature.health = data.health;
    creature.energy = data.energy;
    creature.age = data.age;
    creature.alive = true;  // Loaded creatures are alive
    creature.generation = data.generation;

    creature.foodEaten = data.foodEaten;
    creature.distanceTraveled = data.distanceTraveled;
    creature.successfulHunts = data.successfulHunts;
    creature.escapes = data.escapes;
    creature.wanderAngle = data.wanderAngle;
    creature.animPhase = data.animPhase;

    // Genome
    genome.size = data.genomeSize;
    genome.speed = data.genomeSpeed;
    genome.visionRange = data.genomeVision;
    genome.efficiency = data.genomeEfficiency;
    genome.color.x = data.genomeColorR;
    genome.color.y = data.genomeColorG;
    genome.color.z = data.genomeColorB;
    genome.mutationRate = data.genomeMutationRate;

    // Neural network
    brain.weightsIH = data.weightsIH;
    brain.weightsHO = data.weightsHO;
    brain.biasH = data.biasH;
    brain.biasO = data.biasO;
}

// Helper to build CreatureSnapshot for replay
// Template version for full Creature class
template<typename CreatureType>
CreatureSnapshot buildCreatureSnapshot(const CreatureType& creature) {
    CreatureSnapshot snap;

    snap.id = creature.id;
    snap.type = static_cast<uint8_t>(creature.type);
    snap.posX = creature.position.x;
    snap.posY = creature.position.y;
    snap.posZ = creature.position.z;
    snap.rotation = creature.rotation;
    snap.health = creature.health;
    snap.energy = creature.energy;
    snap.animPhase = creature.animPhase;
    snap.colorR = creature.genome.color.x;
    snap.colorG = creature.genome.color.y;
    snap.colorB = creature.genome.color.z;
    snap.size = creature.genome.size;

    // Genome data for replay
    snap.genomeSpeed = creature.genome.speed;
    snap.genomeSize = creature.genome.size;
    snap.genomeVision = creature.genome.visionRange;

    // Phase 10 Agent 7: Add age and generation for replay visualization
    snap.age = creature.age;
    snap.generation = creature.generation;

    // Neural weights would be populated separately if needed for detailed replay

    return snap;
}


// Helper to build FoodSaveData
template<typename FoodType>
FoodSaveData buildFoodSaveData(const FoodType& food) {
    FoodSaveData data;
    data.posX = food.position.x;
    data.posY = food.position.y;
    data.posZ = food.position.z;
    data.energy = food.energy;
    data.respawnTimer = food.respawnTimer;
    data.active = food.active;
    return data;
}

// Helper to get current Unix timestamp
inline uint64_t getCurrentTimestamp() {
    return static_cast<uint64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
    );
}

// ============================================================================
// SimulationWorld Save/Load Extensions
// ============================================================================

// These are example functions that should be adapted based on actual
// SimulationWorld structure in main.cpp

/*
Usage example - add these to SimulationWorld class:

SaveManager saveManager;
ReplayRecorder replayRecorder;
ReplayPlayer replayPlayer;

// In Initialize():
saveManager.setSaveDirectory(SaveManager::getDefaultSaveDirectory());
saveManager.enableAutoSave(300.0f);  // Auto-save every 5 minutes
saveManager.setAutoSaveCallback([this](const std::string& path) {
    performSave(path);
});

// In Update():
saveManager.update(dt);
if (replayRecorder.isRecording()) {
    ReplayFrame frame = buildCurrentFrame();
    replayRecorder.recordFrame(frame);
    replayRecorder.update(dt, simulationTime);
}

// Save function:
bool performSave(const std::string& filename) {
    SaveFileHeader header;
    header.timestamp = getCurrentTimestamp();
    header.creatureCount = creatures.size();
    header.foodCount = food.size();
    header.generation = generation;
    header.simulationTime = simulationTime;
    header.terrainSeed = 12345;  // Your terrain seed

    WorldSaveData world;
    world.terrainSeed = 12345;
    world.dayTime = dayNight.GetTimeOfDay();
    world.dayDuration = dayNight.GetDayDuration();

    std::vector<CreatureSaveData> creatureData;
    for (const auto& c : creatures) {
        creatureData.push_back(buildCreatureSaveData(*c, c->brain));
    }

    std::vector<FoodSaveData> foodData;
    for (const auto& f : food) {
        foodData.push_back(buildFoodSaveData(*f));
    }

    auto result = saveManager.saveGame(filename, header, world, creatureData, foodData);
    return result == SaveResult::Success;
}

// Load function:
bool performLoad(const std::string& filename) {
    SaveFileHeader header;
    WorldSaveData world;
    std::vector<CreatureSaveData> creatureData;
    std::vector<FoodSaveData> foodData;

    auto result = saveManager.loadGame(filename, header, world, creatureData, foodData);
    if (result != LoadResult::Success) {
        return false;
    }

    // Clear current simulation
    creatures.clear();
    food.clear();

    // Restore terrain (regenerate from seed)
    terrain.Generate(world.terrainSeed);

    // Restore creatures
    for (const auto& data : creatureData) {
        auto creature = std::make_unique<Creature>();
        restoreCreatureFromSaveData(*creature, creature->brain, creature->genome, data);
        creatures.push_back(std::move(creature));
    }

    // Restore food
    for (const auto& data : foodData) {
        auto f = std::make_unique<Food>();
        f->position = Vec3(data.posX, data.posY, data.posZ);
        f->energy = data.energy;
        f->respawnTimer = data.respawnTimer;
        f->active = data.active;
        food.push_back(std::move(f));
    }

    // Restore state
    generation = header.generation;
    simulationTime = header.simulationTime;
    nextCreatureId = header.creatureCount + 1;  // Ensure unique IDs

    dayNight.SetTimeOfDay(world.dayTime);

    return true;
}

// Build replay frame from current state:
ReplayFrame buildCurrentFrame() {
    ReplayFrame frame;
    frame.timestamp = simulationTime;

    for (const auto& c : creatures) {
        frame.creatures.push_back(buildCreatureSnapshot(*c));
    }

    for (const auto& f : food) {
        FoodSnapshot snap;
        snap.posX = f->position.x;
        snap.posY = f->position.y;
        snap.posZ = f->position.z;
        snap.energy = f->energy;
        snap.active = f->active;
        frame.food.push_back(snap);
    }

    // Camera snapshot (from your camera system)
    frame.camera.posX = cameraPosition.x;
    frame.camera.posY = cameraPosition.y;
    frame.camera.posZ = cameraPosition.z;
    frame.camera.targetX = cameraTarget.x;
    frame.camera.targetY = cameraTarget.y;
    frame.camera.targetZ = cameraTarget.z;

    // Statistics
    frame.stats.herbivoreCount = GetHerbivoreCount();
    frame.stats.carnivoreCount = GetCarnivoreCount();
    frame.stats.foodCount = food.size();
    frame.stats.generation = generation;

    return frame;
}
*/

} // namespace Forge

// ============================================================================
// Template Specialization for SimCreature (outside namespace)
// ============================================================================

// Specialized version for SimCreature (handles differences in field names)
// This overload will be used for main.cpp's SimCreature struct
namespace Forge {

// Overload for SimCreature that adapts to its specific field layout
inline CreatureSnapshot buildCreatureSnapshotFromSim(const SimCreature& creature) {
    CreatureSnapshot snap;

    snap.id = creature.id;
    snap.type = static_cast<uint8_t>(creature.type);
    snap.posX = creature.position.x;
    snap.posY = creature.position.y;
    snap.posZ = creature.position.z;
    // SimCreature uses 'facing' vector instead of rotation angle
    snap.rotation = std::atan2(creature.facing.x, creature.facing.z);
    snap.health = creature.energy;  // SimCreature doesn't have separate health
    snap.energy = creature.energy;
    snap.animPhase = 0.0f;  // SimCreature doesn't track animPhase (will be added by Agent 1)
    snap.colorR = creature.genome.color.x;
    snap.colorG = creature.genome.color.y;
    snap.colorB = creature.genome.color.z;
    snap.size = creature.genome.size;

    // Genome data for replay
    snap.genomeSpeed = creature.genome.speed;
    snap.genomeSize = creature.genome.size;
    snap.genomeVision = creature.genome.visionRange;

    // Phase 10 Agent 7: Add age and generation for replay visualization
    // NOTE: These fields will be added to SimCreature by Agent 1
    // For now, use placeholders - Agent 1 will update to use real fields
    snap.age = 0.0f;        // TODO Agent 1: Update to creature.age
    snap.generation = 1;    // TODO Agent 1: Update to creature.generation

    return snap;
}

} // namespace Forge
