# Serialization Research for Evolution Simulator

## Overview

This document presents research findings on serialization approaches for the OrganismEvolution project's save/load and replay systems.

## Project Requirements

### Data to Serialize

Based on analysis of the codebase, the following data structures require serialization:

#### Creature Data (Primary Entity)
- **Position/Physics**: position (Vec3), velocity (Vec3), rotation (float)
- **Life Cycle**: energy, health, age, alive status, generation, id
- **Type**: CreatureType enum (HERBIVORE, CARNIVORE)
- **Fitness Metrics**: fitness, foodEaten, distanceTraveled, successfulHunts, escapes
- **Behavioral State**: wanderAngle, animPhase

#### Genome Data
- **Physical Traits**: size, speed, visionRange, efficiency
- **Visual**: color (RGB), mutationRate
- **Neural Network Weights**: weightsIH, weightsHO, biasH, biasO vectors

#### World State
- **Terrain**: Seed-based regeneration (only store seed)
- **Food**: positions, energy values, respawn timers
- **Simulation**: simulationTime, generation counter, nextCreatureId
- **Day/Night Cycle**: timeOfDay, dayDuration

### Scale Considerations

- Typical simulation: 50-200 creatures
- Maximum supported: 2000 creatures
- Per-creature data: ~300-500 bytes (with neural network weights)
- Total world save: 100KB - 1MB typical, up to 5MB maximum

## Serialization Approaches Compared

### 1. Raw Binary Serialization (Selected Approach)

**Pros:**
- Fastest possible read/write speeds
- Smallest file sizes (42 bytes vs 160 bytes for JSON with same data)
- No external dependencies
- Direct memory mapping possible
- Full control over format

**Cons:**
- No automatic schema evolution
- Platform endianness considerations
- Requires manual versioning

**Performance:**
- Write: ~1-5 MB/s for complex objects
- Read: ~5-20 MB/s depending on complexity
- Save 1000 creatures: <100ms

**Selected because:**
- Project has fixed, known data structures
- Performance is critical for quick save/load
- No need for human-readable save files
- Custom versioning system provides evolution support

### 2. JSON Serialization

**Pros:**
- Human-readable and editable
- Easy debugging
- Schema-flexible
- Excellent for modding support

**Cons:**
- 3-4x larger file sizes than binary
- Parsing overhead significant
- String escaping adds complexity

**Performance:**
- Write: ~100-500 KB/s
- Read: ~200-800 KB/s
- Save 1000 creatures: ~500ms-1s

**Not selected because:**
- No modding support planned
- Performance overhead not acceptable for quick saves

### 3. Cereal Library

**Pros:**
- Header-only, no dependencies
- Supports binary, XML, and JSON archives
- Smart pointer support
- Versioning built-in

**Cons:**
- Learning curve
- Template complexity
- Overkill for our use case

**Not selected because:**
- Project already uses custom patterns
- Binary serialization meets all requirements without library overhead

### 4. MessagePack

**Pros:**
- ~25% smaller than JSON
- ~50% faster than JSON
- Cross-language support
- Schemaless

**Cons:**
- External dependency
- Still parsing overhead
- Not as fast as raw binary

**Not selected because:**
- No cross-language requirements
- Raw binary provides better performance

### 5. Protocol Buffers / FlatBuffers

**Pros:**
- Schema-driven
- Excellent versioning
- Zero-copy deserialization (FlatBuffers)
- Industry standard

**Cons:**
- Requires schema files
- Code generation step
- Complex setup

**Not selected because:**
- Over-engineered for single-application use
- Build complexity not justified

## Recommended Architecture

Based on analysis, we implement a **custom binary serialization system** with:

1. **Chunked File Format**: Magic number, version, and chunk-based layout
2. **Type-Safe Readers/Writers**: Template-based for primitives
3. **Versioning Support**: Version number in header, migration paths
4. **Compression Ready**: Optional zlib compression for large replays

### File Format Design

```
+------------------+
| MAGIC (4 bytes)  |  "EVOS" = 0x534F5645
+------------------+
| VERSION (4 bytes)|  Current: 1
+------------------+
| TIMESTAMP (8)    |  Unix timestamp
+------------------+
| METADATA         |  Creature count, generation, sim time
+------------------+
| CHUNK: WORLD     |  Terrain seed, day/night state
+------------------+
| CHUNK: CREATURES |  All creature data
+------------------+
| CHUNK: FOOD      |  All food data
+------------------+
```

### Neural Network Serialization

The neural network weights are the largest per-creature data:
- weightsIH: INPUT_SIZE * HIDDEN_SIZE = 10 * 12 = 120 floats
- weightsHO: HIDDEN_SIZE * OUTPUT_SIZE = 12 * 4 = 48 floats
- biasH: HIDDEN_SIZE = 12 floats
- biasO: OUTPUT_SIZE = 4 floats

Total: 184 floats * 4 bytes = 736 bytes per creature

Optimization: Could use quantized weights (int8 or int16) to reduce by 75%, but not needed at current scale.

## Replay System Design

### Approach: State Recording (not Input Recording)

We chose state recording over input recording because:
1. Simulation is not strictly deterministic (floating-point, threading)
2. State snapshots allow seeking to any point
3. More robust for replays across code versions

### Frame Structure

Each replay frame captures:
- Timestamp (simulation time)
- All creature positions, rotations, states
- Food positions
- Camera position
- Statistics snapshot

### Recording Strategy

- Default interval: 1 frame per second
- Configurable 0.1s - 5s intervals
- Maximum frames: 36000 (10 hours at 1fps)
- Interpolation for smooth playback between frames

### File Size Estimates

Per frame: ~50 bytes base + (creatures * 50 bytes) + (food * 20 bytes)
Example: 100 creatures, 150 food = 50 + 5000 + 3000 = ~8KB per frame

1 hour replay at 1fps: 3600 frames * 8KB = ~28MB
Compression (optional): ~7MB

## Implementation Details

### Files Created

1. `src/core/Serializer.h` - Binary read/write utilities
2. `src/core/SaveManager.h` - Save/load orchestration
3. `src/core/ReplaySystem.h` - Recording and playback
4. `src/ui/SaveLoadUI.h` - ImGui interface

### Key Classes

- `BinaryWriter` / `BinaryReader` - Type-safe I/O
- `SaveFileHeader` - File metadata
- `CreatureSaveData` - Per-creature serialization
- `SaveManager` - Save slots, auto-save
- `ReplayRecorder` / `ReplayPlayer` - Replay functionality

## Version Migration Strategy

When format changes are needed:

1. Increment `CURRENT_VERSION` in SaveConstants
2. Add migration code in load path:
   ```cpp
   if (header.version < 2) {
       // Migrate v1 to v2
       creature.newField = defaultValue;
   }
   ```
3. Always write latest version

## Performance Benchmarks (Estimated)

| Operation | 100 Creatures | 1000 Creatures | 2000 Creatures |
|-----------|--------------|----------------|----------------|
| Save      | <10ms        | <50ms          | <100ms         |
| Load      | <10ms        | <50ms          | <100ms         |
| Replay Frame | <5ms      | <20ms          | <40ms          |

## Future Considerations

1. **Compression**: Add zlib for large replays
2. **Streaming**: For very long replays, stream from disk
3. **Thumbnails**: Save screenshot with saves for preview
4. **Cloud Sync**: Platform-agnostic save format enables cloud saves

## References

- Game Programming Patterns - Data Locality
- Gaffer on Games - Serialization Strategies
- CppCon - High Performance Serialization
- Game Development Stack Exchange - Save File Design
