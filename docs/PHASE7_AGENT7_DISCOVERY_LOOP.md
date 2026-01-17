# Phase 7 - Agent 7: Discovery, Scanning, and Species Catalog

A No Man's Sky-style discovery loop for OrganismEvolution that makes each run feel like exploration.

## Overview

This system implements a discovery mechanic where players scan and catalog species, unlock information progressively, and experience the thrill of finding rare creatures.

## Components

### 1. SpeciesCatalog (`src/core/SpeciesCatalog.h/.cpp`)

The central data store for discovered species.

#### Data Fields

```cpp
struct SpeciesDiscoveryEntry {
    // Identity
    uint32_t speciesId;
    std::string commonName;
    std::string scientificName;

    // Discovery Metadata
    uint64_t firstSeenTimestamp;    // Unix timestamp
    uint64_t lastSeenTimestamp;
    uint64_t discoveryTimestamp;    // When fully discovered
    float firstSeenSimTime;         // Simulation time

    // Location
    BiomeType discoveryBiome;
    glm::vec3 discoveryLocation;
    std::vector<BiomeType> habitatBiomes;

    // Classification
    CreatureType creatureType;
    RarityTier rarity;              // COMMON to MYTHICAL
    DiscoveryState discoveryState;  // UNDISCOVERED to COMPLETE

    // Statistics
    uint32_t sampleCount;
    uint32_t generationsObserved;
    float averageSize;
    float averageSpeed;

    // Progressive Trait Unlocks
    bool traitsUnlocked[5];
    // 0: Basic (type, color)
    // 1: Physical (size, speed)
    // 2: Behavioral (diet, movement)
    // 3: Environmental (habitat, rarity)
    // 4: Advanced (neural complexity, abilities)

    // Visual
    glm::vec3 primaryColor;
    glm::vec3 secondaryColor;

    // User Notes
    std::string userNotes;
};
```

#### Persistence

The catalog saves/loads via binary serialization using the project's `BinaryWriter`/`BinaryReader` pattern:

```cpp
// Save
catalog.save(writer);

// Load
catalog.load(reader);

// JSON export for debugging
std::string json = catalog.exportToJson();
```

Storage location: Same save directory as game saves (`%APPDATA%/OrganismEvolution/saves/catalog.dat`)

### 2. ScanningSystem (`src/core/ScanningSystem.h/.cpp`)

Handles real-time creature scanning mechanics.

#### Scan Thresholds

| Discovery State | Observation Time | Traits Unlocked |
|-----------------|------------------|-----------------|
| DETECTED        | 0.5 seconds      | Basic (type, color) |
| PARTIAL         | 3.0 seconds      | Physical, Behavioral |
| COMPLETE        | 8.0 seconds      | All traits |

#### Proximity Multipliers

Distance affects scan speed:

| Distance | Multiplier |
|----------|------------|
| 0-10m    | 3.0x       |
| 10-30m   | 1.5x-3.0x  |
| 30-60m   | 1.0x-1.5x  |
| 60m+     | 1.0x       |

Additionally, actively targeting a creature gives a 1.5x bonus.

### 3. Rarity System

Rarity is calculated from genome traits and creature type.

#### Rarity Formula

```cpp
float score = 0.0f;

// Size extremes (+0-15)
if (genome.size < 0.5f || genome.size > 1.8f) score += 15;

// Speed extremes (+0-15)
if (genome.speed < 6.0f || genome.speed > 18.0f) score += 15;

// Special abilities (+0-20 each)
if (genome.hasBioluminescence) score += 20 * intensity;
if (genome.echolocationAbility > 0.5f) score += 15 * ability;
if (genome.electricDischarge > 0.5f) score += 20 * discharge;
if (genome.venomPotency > 0.5f) score += 15 * potency;

// Color uniqueness (+0-10)
if (colorSaturation > 0.6f) score += 10 * saturation;

// Type bonuses
if (AQUATIC_APEX) score += 10;
if (AERIAL_PREDATOR) score += 8;

// Neural complexity (+0-10)
// Camouflage (+0-8)
// Flight/Aquatic specialization bonuses
```

#### Rarity Tiers

| Tier       | Score Range | Color     |
|------------|-------------|-----------|
| COMMON     | 0-24        | Gray      |
| UNCOMMON   | 25-39       | Green     |
| RARE       | 40-54       | Blue      |
| EPIC       | 55-69       | Purple    |
| LEGENDARY  | 70-84       | Orange    |
| MYTHICAL   | 85-100      | Gold      |

### 4. DiscoveryPanel UI (`src/ui/DiscoveryPanel.h/.cpp`)

ImGui-based discovery interface.

#### Features

- Species catalog browser with filters (rarity, biome, type)
- Real-time scan progress overlay (HUD)
- Discovery notifications with rarity colors
- Progressive trait reveal
- User notes per species

#### Configuration

```cpp
struct DiscoveryPanelConfig {
    float panelX = 0.02f;        // 2% from left
    float panelY = 0.1f;         // 10% from top
    float panelWidth = 0.25f;    // 25% screen width
    float panelHeight = 0.6f;    // 60% screen height
    float opacity = 0.85f;
    bool showNotifications = true;
    float notificationDuration = 4.0f;
};
```

### 5. Biome/Theme Naming (`src/entities/SpeciesNameGenerator`)

Extended naming system for biome and planet theme awareness.

#### Biome Prefixes

| Biome Category | Example Prefixes |
|----------------|------------------|
| Ocean/Deep Water | Abyssal, Reef, Trench, Pelagic |
| Coral/Coastal | Coral, Tide, Wave, Kelp |
| Forest | Moss, Fern, Leaf, Willow |
| Grassland | Dawn, Solar, Meadow |
| Cold | Frost, Ice, Glacier, Tundra |
| Volcanic | Ember, Flame, Blaze, Magma |
| Shadow | Shadow, Dusk, Twilight, Void |

#### Planet Theme Prefixes

| Theme | Example Prefixes |
|-------|------------------|
| ALIEN_PURPLE | Violet, Amethyst, Orchid |
| FROZEN_WORLD | Glacier, Permafrost, Arctic |
| VOLCANIC_WORLD | Magma, Obsidian, Pyroclastic |
| BIOLUMINESCENT | Glow, Lumina, Phosphor |
| TOXIC_WORLD | Miasma, Blight, Caustic |

## Example Discovery Record

```json
{
    "speciesId": 42,
    "commonName": "ReefManta",
    "scientificName": "Aquis magnus",
    "rarity": "RARE",
    "discoveryState": "COMPLETE",
    "discoveryBiome": "CORAL_REEF",
    "discoveryLocation": [125.4, 0.0, -89.2],
    "sampleCount": 15,
    "generationsObserved": 8,
    "averageSize": 1.45,
    "averageSpeed": 12.3,
    "traitsUnlocked": [true, true, true, true, true],
    "primaryColor": [0.2, 0.6, 0.9],
    "firstSeenTimestamp": 1705420800
}
```

## Integration Notes for Agent 10 (Dashboard)

### Files NOT to Edit
- `src/ui/SimulationDashboard.*` (owned by Agent 10)
- `src/ui/StatisticsPanel.*` (owned by Agent 10)

### Integration Points

1. **Add DiscoveryPanel Toggle**
   ```cpp
   // In SimulationDashboard, add menu item:
   if (ImGui::MenuItem("Discovery Panel", "Tab")) {
       discoveryPanel.toggle();
   }
   ```

2. **Initialize Discovery System**
   ```cpp
   // In main simulation initialization:
   auto& catalog = discovery::getCatalog();
   auto& scanner = discovery::getScanner();
   scanner.initialize(&catalog);

   ui::DiscoveryPanel discoveryPanel;
   discoveryPanel.initialize(&catalog, &scanner);
   ```

3. **Update Loop Integration**
   ```cpp
   // In main update loop:
   scanner.update(deltaTime, camera, creatures, biomeSystem, simTime, screenW, screenH);
   discoveryPanel.update(deltaTime);

   // In render loop (during ImGui frame):
   discoveryPanel.render(screenW, screenH);
   ```

4. **Record Sightings**
   The scanning system automatically records sightings when creatures are in view. For manual integration:
   ```cpp
   catalog.recordSighting(speciesId, genome, type, biome, position, creatureId, generation, simTime);
   ```

5. **Event Callbacks**
   ```cpp
   catalog.setEventCallback([](const discovery::DiscoveryEvent& event) {
       // Handle discovery events (notifications, achievements, etc.)
       switch (event.type) {
           case DiscoveryEvent::Type::SPECIES_DISCOVERED:
               // Play discovery sound
               break;
           case DiscoveryEvent::Type::RARITY_FOUND:
               // Special effect for rare discoveries
               break;
       }
   });
   ```

## Keyboard Controls (Suggested)

| Key | Action |
|-----|--------|
| Tab | Toggle Discovery Panel |
| E   | Toggle Scanning Mode |
| F   | Lock/Unlock Target |
| Q   | Cycle Targets |

## Statistics Exposed

```cpp
const DiscoveryStatistics& stats = catalog.getStatistics();
stats.speciesDiscovered;        // Total fully discovered
stats.speciesPartiallyScanned;  // Partial scans
stats.totalSightings;           // All sightings
stats.totalScanTime;            // Time spent scanning
stats.rarityCount[6];           // Count per rarity tier
stats.biomeDiscoveries;         // Discoveries per biome
stats.typeDiscoveries;          // Discoveries per creature type
```

## Milestones/Achievements

The system tracks these milestones automatically:
- First Discovery
- 10 Species Discovered (Amateur Naturalist)
- 50 Species Discovered (Field Researcher)
- 100 Species Discovered (Master Cataloger)
- All Rarities Found (Rarity Hunter)
- All Biomes Explored

## Future Enhancements

1. **Audio feedback** - Scan sounds, discovery fanfares
2. **Photo mode** - Capture discovered species
3. **Encyclopedia entries** - Lore text per species
4. **Constellation view** - Visual species relationship map
5. **Discovery sharing** - Export/import catalogs
