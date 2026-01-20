# Known Issues - OrganismEvolution DirectX 12

**Version:** Post-Integration Audit
**Last Updated:** January 16, 2026

This document tracks known issues, their status, workarounds, and planned improvements.

---

## Critical Issues

**Note:** See [INTEGRATION_AUDIT.md](../research/INTEGRATION_AUDIT.md) for the comprehensive 46-issue audit.

### C-01: Neural Network Never Used for Decisions

**Severity:** CRITICAL
**Status:** Architecture Flaw
**Location:** `src/entities/Creature.cpp:46,69,124,157`

**Description:**
Neural network brains are created for every creature but never called. Creature behavior is entirely hardcoded switch statements, not learned behavior.

```cpp
// Brain is CREATED:
brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);

// But NEVER CALLED in behavior updates:
switch (type) {
    case HERBIVORE: updateBehaviorHerbivore(); break;  // Hardcoded logic
    case CARNIVORE: updateBehaviorCarnivore(); break;  // Hardcoded logic
}
```

**Impact:** "Evolution simulator" doesn't actually evolve behavior.

**Workaround:** None - requires integration work.

---

### ~~C-02: Save/Load System Not Wired~~ RESOLVED

**Status:** INTEGRATED
**Location:** `src/main.cpp:3145-3347`

Save/Load system is now integrated. F5 quick save and F9 quick load work. Auto-save runs every 5 minutes.

**Remaining Issue:** Save writes placeholder values for generation, terrain seed. See H-04.

---

### ~~C-03: Replay System Not Recording~~ RESOLVED

**Status:** INTEGRATED
**Location:** `src/main.cpp:4838-4848`

Replay system is now recording frames at 1 fps. F10 toggles replay mode.

---

### C-04: GPU Steering Compute Disabled

**Severity:** MEDIUM (downgraded from CRITICAL)
**Status:** Integrated but Disabled
**Location:** `src/main.cpp:5563-5566`

**Description:**
GPU steering compute is fully integrated but disabled by default due to shader issues. The system falls back to CPU steering.

**Impact:** Works but slower for >200 creatures. GPU path would be faster.

**Current State:** Code at `src/main.cpp:3566-3700` shows full integration. Checkbox in UI exists.

---

### C-05: Animation References Non-Existent Creature Types

**Severity:** CRITICAL
**Status:** Code Bug
**Location:** `src/entities/Creature.cpp:1229-1236`

**Description:**
Animation initialization code references creature types (AVIAN, DEEP_SEA, REEF, PELAGIC) that don't exist in CreatureType.h enum.

**Impact:** May cause compilation errors or undefined behavior.

---

## High Priority Issues

### H-01: Weather Ignores Climate System

**Severity:** HIGH
**Status:** Systems Disconnected
**Location:** `src/environment/WeatherSystem.cpp`

**Description:**
Weather is determined by season + random, completely ignoring the ClimateSystem's temperature/moisture data.

**Impact:** Tropical rainforest can have snow. Weather is not biome-appropriate.

---

### H-02: Vegetation Ignores Biomes

**Severity:** HIGH
**Status:** Systems Disconnected
**Location:** `src/environment/VegetationManager.cpp:68-83`

**Description:**
Tree/vegetation placement uses height only, not biome data. Rainforests look like grasslands.

---

### H-03: Dual Genome System Confusion

**Severity:** HIGH
**Status:** Design Issue
**Location:** `src/entities/Genome.h`, `src/entities/genetics/DiploidGenome.h`

**Description:**
Two parallel genome systems exist (haploid and diploid). Creatures use both inconsistently.

---

### H-04: Save/Load Data Integrity Issues

**Severity:** HIGH
**Status:** Implementation Bug
**Location:** `src/main.cpp:2214-2279`

**Description:**
Save writes placeholder values for generation, terrain seed, and RNG state. Load doesn't restore creature ID counters, causing potential ID collisions.

---

## Medium Priority Issues

See [INTEGRATION_AUDIT.md](../research/INTEGRATION_AUDIT.md) for 17 medium-priority issues including:
- Hardcoded water level in SwimBehavior (-5.0f)
- WingConfig not linked to Genome traits
- Neural topology not evolved
- Empty ClimateSystem.update() function
- Terrain chunking not integrated
- No async compute pipeline

---

## Active Minor Issues

### Creature Shader DEBUG Mode Enabled

**Severity:** Medium
**Status:** Pending Fix
**Location:** `shaders/hlsl/Creature.hlsl` (lines 340-343)

**Description:**
Creatures may render as bright magenta if DEBUG mode is enabled.

**Workaround:**
Comment out line 343 in `Creature.hlsl` to restore normal creature colors.

---

### First-Frame Mouse Jump

**Severity:** Low
**Status:** Mitigated
**Location:** `src/main.cpp` (ProcessInput function)

**Description:**
On initial mouse capture, there may be a small camera jump.

**Mitigation Applied:**
- 2-frame skip after initial capture
- Delta clamping (+-50 pixels)
- Dead zone filtering

---

## Build/Configuration Issues

### Shader Directory Duplication

**Severity:** Medium
**Location:** `Runtime/Shaders/` vs `shaders/hlsl/`

**Description:**
Two shader directories exist. Only `Runtime/Shaders` is copied by the build. Edits to `shaders/hlsl/` won't ship.

**Question:** Which is the source of truth?

---

### Tests Not Wired to Build

**Severity:** Low
**Location:** `tests/`, `CMakeLists.txt`

**Description:**
Test files exist but no `enable_testing()` or `add_subdirectory(tests)` in CMake.

---

## Issue Summary (Updated Jan 16, 2026)

| Category | Critical | High | Medium | Low | Total |
|----------|----------|------|--------|-----|-------|
| Neural/AI | 1 | 0 | 2 | 0 | 3 |
| Save/Replay | 0 (resolved) | 1 | 1 | 0 | 2 |
| Environment | 0 | 2 | 5 | 1 | 8 |
| Creatures | 1 | 1 | 5 | 2 | 9 |
| Rendering | 0 | 0 | 1 | 3 | 4 |
| Build/Config | 0 | 0 | 2 | 2 | 4 |
| **Total** | **2** | **4** | **16** | **8** | **30** |

**Resolved since audit:** Save/Load integrated, Replay integrated, GPU steering integrated (but disabled)

---

## Honest Feature Status (Updated Jan 16, 2026)

| Feature | Marketing Claim | Reality |
|---------|-----------------|---------|
| "Evolution simulator" | Genetic evolution | Basic trait inheritance only |
| "Neural network brains" | AI-driven behavior | **Code exists, NOT used** |
| "Save/Load system" | Persistence | **WORKING** - F5/F9 functional |
| "Replay system" | Record/playback | **WORKING** - F10 functional |
| "GPU accelerated" | Compute shaders | Rendering + steering (disabled) |
| "Ecosystem simulation" | Complex food web | Basic predator-prey only |
| "10,000 creatures" | Massive scale | Possible but slow (15 FPS) |

---

## Related Documents

- **[INTEGRATION_AUDIT.md](../research/INTEGRATION_AUDIT.md)** - Comprehensive 46-issue audit
- **[PROJECT_ISSUES.md](PROJECT_ISSUES.md)** - Build and data integrity issues
- **[../../KNOWN_ISSUES.md](../../KNOWN_ISSUES.md)** - Root-level issues document (more current)

---

*Last updated: January 19, 2026*
*Updated to reflect actual project state per integration audit*
