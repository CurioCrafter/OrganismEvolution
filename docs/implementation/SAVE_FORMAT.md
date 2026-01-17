# Save File Format Specification

## Overview

The Evolution Simulator uses a custom binary save format (.evos) designed for fast loading, compact size, and forward compatibility.

## File Extension

- Save files: `.evos` (Evolution Save)
- Replay files: `.evor` (Evolution Replay) - uses `.evos` internally with replay chunk

## Binary Format Structure

### Byte Order

All multi-byte values are stored in **little-endian** format (native x86/x64).

### File Layout

```
┌─────────────────────────────────────────────────────────────┐
│                     FILE HEADER (44 bytes)                  │
├─────────────────────────────────────────────────────────────┤
│                     CHUNK: WORLD                            │
├─────────────────────────────────────────────────────────────┤
│                     CHUNK: CREATURES                        │
├─────────────────────────────────────────────────────────────┤
│                     CHUNK: FOOD                             │
├─────────────────────────────────────────────────────────────┤
│                     [OPTIONAL: CHUNK: REPLAY]               │
└─────────────────────────────────────────────────────────────┘
```

## File Header

| Offset | Size | Type     | Field           | Description                    |
|--------|------|----------|-----------------|--------------------------------|
| 0      | 4    | uint32   | magic           | "EVOS" = 0x534F5645            |
| 4      | 4    | uint32   | version         | File format version (1)        |
| 8      | 8    | uint64   | timestamp       | Unix timestamp when saved      |
| 16     | 4    | uint32   | creatureCount   | Number of creatures in file    |
| 20     | 4    | uint32   | foodCount       | Number of food items           |
| 24     | 4    | uint32   | generation      | Current generation number      |
| 28     | 4    | float    | simulationTime  | Total simulation time (seconds)|
| 32     | 4    | uint32   | terrainSeed     | Terrain generation seed        |
| 36     | 4    | uint32   | flags           | Reserved for future use        |

**Total Header Size: 40 bytes**

## Chunk Format

Each chunk begins with a 4-byte identifier:

| Chunk ID     | Value      | Description                |
|--------------|------------|----------------------------|
| CHUNK_WORLD  | 0x574C4400 | World state data           |
| CHUNK_CREATURES | 0x43525400 | Creature data array     |
| CHUNK_FOOD   | 0x464F4F44 | Food data array            |
| CHUNK_REPLAY | 0x52504C59 | Replay frames (optional)   |

### World Chunk

| Offset | Size | Type   | Field        | Description                    |
|--------|------|--------|--------------|--------------------------------|
| 0      | 4    | uint32 | terrainSeed  | Seed for terrain regeneration  |
| 4      | 4    | float  | dayTime      | Time of day (0-1)              |
| 8      | 4    | float  | dayDuration  | Seconds per day cycle          |
| 12     | 4    | uint32 | rngState     | Random number generator state  |

**World Chunk Size: 16 bytes**

### Creatures Chunk

Format: `[chunk_id:4][count:4][creature_data * count]`

#### Creature Data Structure

| Offset | Size | Type   | Field             | Description                    |
|--------|------|--------|-------------------|--------------------------------|
| 0      | 4    | uint32 | id                | Unique creature ID             |
| 4      | 1    | uint8  | type              | CreatureType enum              |
| 5      | 4    | float  | posX              | Position X                     |
| 9      | 4    | float  | posY              | Position Y                     |
| 13     | 4    | float  | posZ              | Position Z                     |
| 17     | 4    | float  | velX              | Velocity X                     |
| 21     | 4    | float  | velY              | Velocity Y                     |
| 25     | 4    | float  | velZ              | Velocity Z                     |
| 29     | 4    | float  | rotation          | Facing direction               |
| 33     | 4    | float  | health            | Current health                 |
| 37     | 4    | float  | energy            | Current energy                 |
| 41     | 4    | float  | age               | Age in seconds                 |
| 45     | 4    | int32  | generation        | Generation number              |
| 49     | 4    | float  | foodEaten         | Total food consumed            |
| 53     | 4    | float  | distanceTraveled  | Total distance                 |
| 57     | 4    | int32  | successfulHunts   | Hunt count (carnivores)        |
| 61     | 4    | int32  | escapes           | Escape count (herbivores)      |
| 65     | 4    | float  | wanderAngle       | Wander behavior state          |
| 69     | 4    | float  | animPhase         | Animation phase                |
| 73     | 4    | float  | genomeSize        | Genome: size trait             |
| 77     | 4    | float  | genomeSpeed       | Genome: speed trait            |
| 81     | 4    | float  | genomeVision      | Genome: vision range           |
| 85     | 4    | float  | genomeEfficiency  | Genome: efficiency             |
| 89     | 4    | float  | genomeColorR      | Genome: color red              |
| 93     | 4    | float  | genomeColorG      | Genome: color green            |
| 97     | 4    | float  | genomeColorB      | Genome: color blue             |
| 101    | 4    | float  | genomeMutationRate| Genome: mutation rate          |
| 105    | 4    | uint32 | weightsIHCount    | Neural weights IH count        |
| 109    | var  | float[]| weightsIH         | Input->Hidden weights          |
| var    | 4    | uint32 | weightsHOCount    | Neural weights HO count        |
| var    | var  | float[]| weightsHO         | Hidden->Output weights         |
| var    | 4    | uint32 | biasHCount        | Hidden bias count              |
| var    | var  | float[]| biasH             | Hidden layer biases            |
| var    | 4    | uint32 | biasOCount        | Output bias count              |
| var    | var  | float[]| biasO             | Output layer biases            |

**Creature Base Size: 105 bytes + neural network weights**
**Typical Creature Size: ~850 bytes (with 184 weights)**

### Food Chunk

Format: `[chunk_id:4][count:4][food_data * count]`

#### Food Data Structure

| Offset | Size | Type  | Field        | Description                    |
|--------|------|-------|--------------|--------------------------------|
| 0      | 4    | float | posX         | Position X                     |
| 4      | 4    | float | posY         | Position Y                     |
| 8      | 4    | float | posZ         | Position Z                     |
| 12     | 4    | float | energy       | Energy value                   |
| 16     | 4    | float | respawnTimer | Time until respawn             |
| 20     | 1    | uint8 | active       | Is food active (0/1)           |

**Food Data Size: 21 bytes**

## Replay Format

Replays are stored either in a separate file or appended to a save file.

### Replay Header

| Offset | Size | Type   | Field          | Description                    |
|--------|------|--------|----------------|--------------------------------|
| 0      | 4    | uint32 | magic          | "RPLY" = 0x52504C59            |
| 4      | 4    | uint32 | version        | Replay format version          |
| 8      | 8    | uint64 | timestamp      | Recording start timestamp      |
| 16     | 4    | uint32 | terrainSeed    | Terrain seed for playback      |
| 20     | 4    | uint32 | frameCount     | Total number of frames         |
| 24     | 4    | float  | duration       | Total replay duration          |
| 28     | 4    | float  | recordInterval | Seconds between frames         |

**Replay Header Size: 32 bytes**

### Replay Frame

Each frame contains a snapshot of the simulation state:

```
┌─────────────────────────────────────────────┐
│ timestamp (float, 4 bytes)                  │
├─────────────────────────────────────────────┤
│ creatureCount (uint32, 4 bytes)             │
│ [CreatureSnapshot * creatureCount]          │
├─────────────────────────────────────────────┤
│ foodCount (uint32, 4 bytes)                 │
│ [FoodSnapshot * foodCount]                  │
├─────────────────────────────────────────────┤
│ CameraSnapshot (28 bytes)                   │
├─────────────────────────────────────────────┤
│ StatisticsSnapshot (24 bytes)               │
└─────────────────────────────────────────────┘
```

#### Creature Snapshot (48 bytes)

| Offset | Size | Type  | Field    | Description           |
|--------|------|-------|----------|-----------------------|
| 0      | 4    | uint32| id       | Creature ID           |
| 4      | 1    | uint8 | type     | Creature type         |
| 5      | 4    | float | posX     | Position X            |
| 9      | 4    | float | posY     | Position Y            |
| 13     | 4    | float | posZ     | Position Z            |
| 17     | 4    | float | rotation | Facing direction      |
| 21     | 4    | float | health   | Health value          |
| 25     | 4    | float | energy   | Energy value          |
| 29     | 4    | float | animPhase| Animation state       |
| 33     | 4    | float | colorR   | Display color R       |
| 37     | 4    | float | colorG   | Display color G       |
| 41     | 4    | float | colorB   | Display color B       |
| 45     | 4    | float | size     | Display size          |

#### Food Snapshot (17 bytes)

| Offset | Size | Type  | Field  | Description           |
|--------|------|-------|--------|-----------------------|
| 0      | 4    | float | posX   | Position X            |
| 4      | 4    | float | posY   | Position Y            |
| 8      | 4    | float | posZ   | Position Z            |
| 12     | 4    | float | energy | Energy value          |
| 16     | 1    | uint8 | active | Active state          |

#### Camera Snapshot (28 bytes)

| Offset | Size | Type  | Field   | Description           |
|--------|------|-------|---------|----------------------|
| 0      | 4    | float | posX    | Camera position X    |
| 4      | 4    | float | posY    | Camera position Y    |
| 8      | 4    | float | posZ    | Camera position Z    |
| 12     | 4    | float | targetX | Look-at target X     |
| 16     | 4    | float | targetY | Look-at target Y     |
| 20     | 4    | float | targetZ | Look-at target Z     |
| 24     | 4    | float | fov     | Field of view        |

#### Statistics Snapshot (24 bytes)

| Offset | Size | Type   | Field              | Description           |
|--------|------|--------|--------------------|-----------------------|
| 0      | 4    | uint32 | herbivoreCount     | Number of herbivores  |
| 4      | 4    | uint32 | carnivoreCount     | Number of carnivores  |
| 8      | 4    | uint32 | foodCount          | Food items active     |
| 12     | 4    | uint32 | generation         | Current generation    |
| 16     | 4    | float  | avgHerbivoreFitness| Herbivore avg fitness |
| 20     | 4    | float  | avgCarnivoreFitness| Carnivore avg fitness |

## Version History

| Version | Changes                                    |
|---------|-------------------------------------------|
| 1       | Initial format                            |

## File Size Estimates

| Scenario          | Creatures | Food | Save Size | Replay/hour |
|-------------------|-----------|------|-----------|-------------|
| Small simulation  | 50        | 100  | ~50 KB    | ~15 MB      |
| Medium simulation | 200       | 150  | ~180 KB   | ~50 MB      |
| Large simulation  | 1000      | 200  | ~900 KB   | ~250 MB     |
| Maximum           | 2000      | 300  | ~1.8 MB   | ~500 MB     |

## Implementation Notes

### Validation

On load, the following checks are performed:
1. Magic number matches "EVOS"
2. Version is <= CURRENT_VERSION
3. Chunk IDs appear in expected order
4. Counts match actual data sizes

### Error Handling

- Invalid magic: `LoadResult::InvalidFormat`
- Future version: `LoadResult::VersionMismatch`
- Truncated file: `LoadResult::CorruptedData`
- I/O failure: `LoadResult::ReadError`

### Thread Safety

Save/load operations are NOT thread-safe. The simulation should be paused during save/load operations.

### Save Directory

Default save locations:
- Windows: `%APPDATA%\OrganismEvolution\saves\`
- Linux: `~/.local/share/OrganismEvolution/saves/`

## Example: Reading a Save File (Pseudocode)

```cpp
BinaryReader reader;
reader.open("save.evos");

SaveFileHeader header;
header.read(reader);

if (header.magic != MAGIC_NUMBER) {
    return LoadResult::InvalidFormat;
}

if (header.version > CURRENT_VERSION) {
    return LoadResult::VersionMismatch;
}

// Read world chunk
uint32_t chunkId = reader.read<uint32_t>();
assert(chunkId == CHUNK_WORLD);
WorldSaveData world;
world.read(reader);

// Read creatures chunk
chunkId = reader.read<uint32_t>();
assert(chunkId == CHUNK_CREATURES);
uint32_t count = reader.read<uint32_t>();
for (uint32_t i = 0; i < count; ++i) {
    CreatureSaveData creature;
    creature.read(reader);
    loadCreature(creature);
}

// Read food chunk
chunkId = reader.read<uint32_t>();
assert(chunkId == CHUNK_FOOD);
count = reader.read<uint32_t>();
for (uint32_t i = 0; i < count; ++i) {
    FoodSaveData food;
    food.read(reader);
    loadFood(food);
}

reader.close();
```

## Compatibility Guidelines

When adding new fields to the format:

1. Increment the version number
2. Add new fields at the END of structures
3. Provide default values for missing fields on load
4. Document changes in Version History table

Example migration:
```cpp
if (header.version < 2) {
    // New field added in v2 - use default
    creature.newField = 0.0f;
} else {
    creature.newField = reader.read<float>();
}
```
