# Phase 7 Agent 6: Ecosystem Depth and Food Web Complexity

## Summary

This implementation adds ecosystem depth through new resource types, enhanced nutrient cycling loops, seasonal blooms, and scarcity signals for behaviors. The food web visualization has been extended to show the complete energy flow from nutrients through decomposers back to producers.

---

## New Resource Types and Flows

### 1. Detritus System (New)

**Location:** `src/environment/ProducerSystem.h/.cpp`

Detritus represents dead organic matter (fallen leaves, dead roots, plant debris) separate from creature corpses. It forms the base of the decomposer food chain.

**Fields Added to SoilTile:**
```cpp
float detritus;  // 0-100, dead organic matter level
```

**Key Functions:**
- `addDetritus(position, amount)` - Add dead matter to soil
- `getDetritusAt(position, radius)` - Query local detritus density
- `consumeDetritus(position, amount)` - Decomposers/scavengers consume detritus
- `getDetritusHotspots()` - Returns high-detritus positions for scavenger AI
- `updateDetritus(deltaTime)` - Handles decay and natural accumulation

**Detritus Flow:**
```
Dead Plants/Leaf Fall → Detritus → Decomposers → Nutrients → Producers
                                 ↘ Scavengers (energy)
```

### 2. Enhanced Scavenger Loop

**Location:** `src/environment/DecomposerSystem.h/.cpp`

Scavengers can now target both corpses AND detritus hotspots, creating a more resilient food source during periods of low creature mortality.

**New Functions:**
- `getCarrionDensity(position, radius)` - Combined corpse + detritus density
- `getScavengingTargets()` - All valid scavenging positions
- `getTotalCarrionBiomass()` - Ecosystem metric for total carrion

### 3. Seasonal Bloom System

**Location:** `src/environment/ProducerSystem.h/.cpp`

Producers now have seasonal productivity blooms that create pulses of abundance.

**Bloom Types:**
| Type | Season | Multiplier | Description |
|------|--------|------------|-------------|
| 0 | - | 1.0x | No bloom (baseline) |
| 1 | Spring | up to 1.5x | Spring growth bloom (mid-spring peak) |
| 2 | Fall | up to 1.3x | Fungal burst (mushroom season) |
| 3 | Winter | up to 1.2x | Brief plankton bloom (early winter) |

**Query Functions:**
- `getSeasonalBloomMultiplier()` - Current production multiplier
- `isInBloomPeriod()` - True if multiplier > 1.2
- `getBloomType()` - Active bloom type (0-3)

### 4. Nutrient Recycling Enhancement

The decomposer → producer feedback loop has been strengthened:

```cpp
// In DecomposerSystem::releaseNutrients()
float feedbackAmount = decomposedAmount * nutrientFeedbackRate;
producerSystem->addNutrients(position, nitrogen, phosphorus, organicMatter);
producerSystem->addDetritus(position, feedbackAmount * 0.15f);
```

---

## EcosystemSignals Struct

**Location:** `src/environment/EcosystemManager.h`

Read-only scarcity and abundance indicators exposed for behavior queries without modifying ecosystem state.

### Signal Fields

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `plantFoodPressure` | float | 0-1 | Scarcity of plant food (0=abundant, 1=scarce) |
| `preyPressure` | float | 0-1 | Scarcity of prey for carnivores |
| `carrionDensity` | float | 0-1 | Availability of carrion/detritus |
| `producerBiomass` | float | 0-1 | Overall plant biomass availability |
| `detritusLevel` | float | 0-1 | Detritus for decomposers |
| `nutrientSaturation` | float | 0-1 | Soil nutrient levels |
| `herbivorePopulationPressure` | float | -0.5 to 1.5 | Population vs carrying capacity (0=at target) |
| `carnivorePopulationPressure` | float | -0.5 to 1.5 | Population vs carrying capacity |
| `seasonalBloomStrength` | float | 0.5-1.5 | Current bloom multiplier |
| `activeBloomType` | int | 0-3 | Type of active bloom |
| `isWinter` | bool | - | Simple winter flag |
| `dayLengthFactor` | float | 0.5-1.5 | Day length relative to 12 hours |
| `localCompetition` | float | 0-1 | Local competition intensity |
| `predationRisk` | float | 0-1 | Local predation risk |

### Query Functions

```cpp
// Global signals (cached, fast)
const EcosystemSignals& getSignals() const;

// Local signals (position-specific, slower)
EcosystemSignals getLocalSignals(const glm::vec3& position, float radius) const;

// Type-specific food pressure
float getFoodPressure(CreatureType forType) const;

// Carrion availability (for scavengers)
float getCarrionAvailability() const;

// Predation risk at position
float getPredationRisk(const glm::vec3& position) const;
```

### Usage Example (for behaviors - read-only)

```cpp
// In behavior code (no editing behavior files required per parallel safety)
const EcosystemSignals& signals = ecosystemManager->getSignals();

if (signals.plantFoodPressure > 0.7f) {
    // Food is scarce - increase foraging range
}

if (signals.isWinter && signals.carrionDensity < 0.2f) {
    // Winter with low carrion - consider migration
}

if (signals.activeBloomType == 1) {
    // Spring bloom active - good time to reproduce
}
```

---

## Food Web Visualization Updates

**Location:** `src/ui/statistics/FoodWebViz.h/.cpp`

### New Node Types

```cpp
enum class FoodWebNodeType {
    CREATURE,      // Standard creature (circles)
    PRODUCER,      // Plants (diamonds)
    DETRITUS,      // Dead matter (rounded squares)
    PLANKTON,      // Aquatic producers (diamonds)
    DECOMPOSER,    // Fungi (diamonds)
    NUTRIENT       // Soil nutrients (rounded squares)
};
```

### Visual Encoding

| Node Type | Shape | Base Color |
|-----------|-------|------------|
| Creatures | Circle | Type-specific |
| Producers | Diamond | Green (0.2, 0.7, 0.2) |
| Plankton | Diamond | Light Blue (0.3, 0.6, 0.8) |
| Decomposers | Diamond | Purple-brown (0.7, 0.5, 0.6) |
| Detritus | Rounded Square | Dark Brown (0.5, 0.35, 0.2) |
| Nutrients | Rounded Square | Brown (0.6, 0.4, 0.2) |

### Bloom Indicators

- **Spring Bloom:** Producers glow brighter green
- **Fungal Burst:** Decomposers glow brighter purple
- **Plankton Bloom:** Plankton glows brighter blue
- **Header indicator:** Shows active bloom type and multiplier

### New Edges (Energy Flow)

```
Nutrients → Producers (40% efficiency)
Producers → Herbivores (20% efficiency)
Plankton → Grazers (15% efficiency)
Detritus → Scavengers (30% efficiency)
Detritus → Decomposers (50% efficiency)
Decomposers → Nutrients (60% efficiency - recycling)
All Creatures → Detritus (50% on death)
```

### New Update Function

```cpp
void updateResources(float producerBiomass, float detritusLevel,
                    float decomposerActivity, float planktonLevel,
                    float bloomMultiplier, int bloomType);
```

---

## Files Modified

### Core Changes
- `src/environment/ProducerSystem.h` - Added detritus field, seasonal bloom functions
- `src/environment/ProducerSystem.cpp` - Implemented detritus and bloom systems
- `src/environment/DecomposerSystem.h` - Added enhanced scavenger loop functions
- `src/environment/DecomposerSystem.cpp` - Implemented carrion density and scavenging targets
- `src/environment/EcosystemManager.h` - Added EcosystemSignals struct and accessors
- `src/environment/EcosystemManager.cpp` - Implemented signal calculation and updates

### Visualization
- `src/ui/statistics/FoodWebViz.h` - Added FoodWebNodeType, resource node tracking
- `src/ui/statistics/FoodWebViz.cpp` - Resource nodes, new edges, bloom indicators

---

## Handoff Notes

### For Agent 4 (Behaviors):
The `EcosystemSignals` struct provides read-only access to scarcity indicators. Behaviors can query:
- Food pressure by creature type
- Carrion availability for scavengers
- Seasonal bloom state for reproduction decisions
- Winter flag for dormancy/migration triggers

No behavior code modifications were made per parallel safety rules. The signals are designed for behaviors to query without coupling.

### For Agent 10 (Dashboard):
The FoodWebViz now has additional nodes and a new `updateResources()` function that should be called with ecosystem data. Example integration:

```cpp
foodWebViz.updateResources(
    ecosystemManager->getProducers()->getTotalBiomass(),
    ecosystemManager->getSignals().detritusLevel,
    ecosystemManager->getDecomposers()->getDecompositionRate(),
    0.5f,  // plankton level (from aquatic system)
    ecosystemManager->getSignals().seasonalBloomStrength,
    ecosystemManager->getSignals().activeBloomType
);
```

---

## Validation Checklist

- [x] Detritus accumulates from natural plant death and fall leaf drop
- [x] Decomposers consume detritus and release nutrients to soil
- [x] Nutrients feed back into producer growth rate
- [x] Seasonal blooms create productivity pulses
- [x] Scavengers can target both corpses and detritus hotspots
- [x] EcosystemSignals updated periodically (every 0.5s)
- [x] Food web visualization shows complete nutrient cycle
- [x] Bloom indicators visible during active blooms
