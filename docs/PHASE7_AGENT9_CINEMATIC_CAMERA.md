# Phase 7 - Agent 9: Cinematic Camera and Presentation

## Overview

This document describes the cinematic camera system implemented for the OrganismEvolution simulation. The system provides professional-quality camera presentation modes that make the simulation engaging to watch and explore. The implementation extends the existing `CameraController` class to maintain compatibility while adding new cinematic capabilities.

## Files Modified

### Owned Files (Agent 9)
- `src/graphics/CameraController.h` - **MODIFIED** - Added cinematic modes, target selection, collision avoidance, photo mode
- `src/graphics/CameraController.cpp` - **MODIFIED** - Implemented all new functionality

### Files Read (Reference Only)
- `src/graphics/Camera.h` - Base camera class
- `src/graphics/Camera.cpp` - Base camera implementation
- `src/graphics/IslandCamera.h` - Island navigation camera
- `src/graphics/IslandCamera.cpp` - Island navigation implementation
- `src/environment/Terrain.h` - Terrain for collision detection

### Integration Points for Agent 10
- Exposed `onPhotoModeFreeze` callback for UI integration
- Exposed target override API for UI-driven camera control
- Exposed camera mode setters for dashboard integration

---

## Camera Modes

### Existing Modes (Preserved)

| Mode | Description |
|------|-------------|
| `FREE` | Standard WASD + mouse free-look |
| `FOLLOW` | Follow creature from behind |
| `ORBIT` | Orbit around a point/creature |
| `UNDERWATER` | Underwater with depth effects |
| `FLYING` | Bird's eye view |
| `CINEMATIC` | Keyframe-based cinematic path |
| `OVERVIEW` | Top-down world view |
| `FIRST_PERSON` | First-person from creature POV |

### New Cinematic Modes (Phase 7)

| Mode | Description |
|------|-------------|
| `CINEMATIC_SLOW_ORBIT` | Dramatic slow orbit around target with vertical variation |
| `CINEMATIC_GLIDE` | Smooth gliding motion through the scene |
| `CINEMATIC_FOLLOW_TARGET` | Intelligent creature tracking with dynamic framing |
| `PHOTO_MODE` | Frozen camera with manual adjustments |

---

## Configuration Structures

### CinematicCameraConfig

```cpp
struct CinematicCameraConfig {
    // Movement parameters
    float orbitSpeed = 0.15f;           // Radians/sec for orbit modes
    float glideSpeed = 8.0f;            // Units/sec for glide mode
    float heightVariation = 0.3f;       // Vertical movement amplitude (0-1)

    // Distance and framing
    float minDistance = 15.0f;          // Minimum distance from target
    float maxDistance = 80.0f;          // Maximum distance from target
    float preferredDistance = 35.0f;    // Default cinematic distance
    float heightOffset = 8.0f;          // Default height above target

    // Smoothing (critically damped spring)
    float positionSmoothTime = 0.8f;    // Position smoothing
    float rotationSmoothTime = 0.5f;    // Rotation smoothing
    float fovSmoothTime = 1.5f;         // FOV transition smoothing

    // Presentation effects
    float baseFOV = 45.0f;              // Default field of view
    float cinematicFOV = 35.0f;         // Narrower FOV for drama
    float maxFOVChange = 15.0f;         // Maximum FOV variation
    float rollIntensity = 0.02f;        // Camera roll (radians)
    float rollSpeed = 0.3f;             // Roll oscillation speed

    // Collision avoidance
    float collisionPadding = 2.0f;      // Distance from terrain
    float underwaterAvoidanceMargin = 1.0f;
    bool avoidUnderwater = true;
    bool avoidTerrain = true;
};
```

### TargetSelectionConfig

```cpp
struct TargetSelectionConfig {
    TargetSelectionMode mode = MANUAL;
    float switchInterval = 15.0f;       // Seconds between auto-switches
    float actionThreshold = 5.0f;       // Velocity for "action" detection
    float smoothTransitionTime = 2.0f;  // Transition time between targets
    bool lockTarget = false;            // Prevent auto-switching
};
```

---

## Target Selection System

### Selection Heuristics

| Mode | Description |
|------|-------------|
| `MANUAL` | User-selected target only |
| `LARGEST_CREATURE` | Auto-select biggest creature |
| `NEAREST_ACTION` | Select fastest-moving creature |
| `RANDOM_FOCUS` | Periodically switch randomly |
| `PREDATOR_PRIORITY` | Prioritize large, fast creatures |
| `MOST_OFFSPRING` | Select most prolific creature |

### API for Target Selection

```cpp
// Set selection mode
TargetSelectionConfig config = controller.getTargetSelectionConfig();
config.mode = TargetSelectionMode::LARGEST_CREATURE;
controller.setTargetSelectionConfig(config);

// Override/lock a specific target (UI control)
controller.overrideTarget(creature);
controller.lockTarget(true);

// Clear override to allow automatic selection
controller.clearTargetOverride();

// Check if target is locked
bool locked = controller.isTargetLocked();

// Get current target
const Creature* target = controller.getCurrentTarget();
glm::vec3 targetPos = controller.getCurrentTargetPosition();

// Provide creature pool for auto-selection
controller.setCreaturePool(&creatureVector);
```

---

## Collision and Terrain Avoidance

The camera automatically avoids terrain and water clipping.

### Setup

```cpp
controller.setTerrain(terrain);
controller.setWaterLevel(waterLevel);
```

### Configuration

```cpp
CinematicCameraConfig config = controller.getCinematicConfig();
config.collisionPadding = 3.0f;        // Distance above terrain
config.underwaterAvoidanceMargin = 2.0f; // Distance above water
config.avoidUnderwater = true;
config.avoidTerrain = true;
controller.setCinematicConfig(config);
```

### Manual Collision Check

```cpp
glm::vec3 safePos = controller.applyCollisionAvoidance(desiredPosition);
bool willCollide = controller.wouldCollide(testPosition);
```

---

## Photo Mode

Freezes camera movement for screenshot composition.

### API

```cpp
// Enter photo mode
controller.enterPhotoMode();

// Exit photo mode
controller.exitPhotoMode();

// Check state
bool inPhotoMode = controller.isInPhotoMode();
const PhotoModeState& state = controller.getPhotoModeState();

// Manual controls
controller.setPhotoModeFOV(35.0f);           // 10-120 degrees
controller.setPhotoModeRoll(0.05f);          // -0.5 to 0.5 radians
controller.setPhotoModeFocus(50.0f, 0.5f);   // Distance, strength
controller.photoModeNudge(glm::vec3(1, 0, 0)); // Small position adjustment
```

### Photo Mode State Structure

```cpp
struct PhotoModeState {
    bool active = false;
    bool freezeCamera = true;
    bool freezeSimulation = false;
    float manualFOV = 45.0f;
    float manualRoll = 0.0f;
    glm::vec3 frozenPosition;
    glm::vec3 frozenTarget;
    float depthOfFieldFocus = 50.0f;
    float depthOfFieldStrength = 0.0f;
};
```

---

## Presentation Effects

### Dynamic FOV

- Narrow FOV (35-40) for dramatic close-ups during fast action
- Base FOV (45) for normal viewing
- Smooth transitions between FOV values using critically damped springs

### Camera Roll

Subtle roll oscillation adds cinematic feel:
- Configurable via `rollIntensity` (default 0.02 radians)
- Oscillation speed via `rollSpeed` (default 0.3)
- Accessible via `getCinematicRoll()` for shader integration

---

## Starting Cinematic Modes

```cpp
// Slow dramatic orbit around a center point
controller.startSlowOrbit(worldCenter, 50.0f);  // -1 uses config default

// Smooth glide from start to end position
controller.startGlide(startPos, endPos, 10.0f);  // duration in seconds

// Intelligent creature following
controller.startFollowTarget(selectedCreature);
```

---

## Integration Notes for Agent 10 UI

### Photo Mode Callback

Register a callback to receive photo mode freeze notifications:

```cpp
controller.onPhotoModeFreeze = [](bool freeze) {
    if (freeze) {
        // Pause simulation
        simulation.setPaused(true);
    } else {
        // Resume simulation
        simulation.setPaused(false);
    }
};
```

### UI Integration Example

```cpp
// In SimulationDashboard (Agent 10)

// Camera mode buttons
if (ImGui::Button("Slow Orbit")) {
    controller.startSlowOrbit(worldCenter, 50.0f);
}

if (ImGui::Button("Follow Creature")) {
    if (selectedCreature) {
        controller.startFollowTarget(selectedCreature);
    }
}

if (ImGui::Button("Glide Tour")) {
    controller.startGlide(tourStart, tourEnd, 15.0f);
}

// Target selection dropdown
const char* modes[] = {"Manual", "Largest", "Most Active", "Random", "Predator"};
static int currentMode = 0;
if (ImGui::Combo("Auto-Target", &currentMode, modes, 5)) {
    TargetSelectionConfig config = controller.getTargetSelectionConfig();
    config.mode = static_cast<TargetSelectionMode>(currentMode);
    controller.setTargetSelectionConfig(config);
}

// Override target from UI selection
if (ImGui::Button("Focus Selected")) {
    controller.overrideTarget(uiSelectedCreature);
}

// Photo mode controls
if (controller.isInPhotoMode()) {
    const auto& state = controller.getPhotoModeState();

    float fov = state.manualFOV;
    if (ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f)) {
        controller.setPhotoModeFOV(fov);
    }

    float roll = state.manualRoll;
    if (ImGui::SliderFloat("Roll", &roll, -0.5f, 0.5f)) {
        controller.setPhotoModeRoll(roll);
    }

    // Nudge controls
    if (ImGui::Button("Left"))  controller.photoModeNudge(glm::vec3(-1, 0, 0));
    if (ImGui::Button("Right")) controller.photoModeNudge(glm::vec3(1, 0, 0));
    if (ImGui::Button("Up"))    controller.photoModeNudge(glm::vec3(0, 1, 0));
    if (ImGui::Button("Down"))  controller.photoModeNudge(glm::vec3(0, -1, 0));
}
```

---

## Technical Details

### Critically Damped Smoothing

All camera movements use critically damped spring interpolation (Unity-style SmoothDamp) for smooth, natural motion without overshoot:

```cpp
// Position smoothing
m_camera->Position = smoothDamp(m_camera->Position, desiredPos,
                                m_positionVelocity,
                                m_cinematicConfig.positionSmoothTime, deltaTime);
```

### Smoothing Parameters

| Parameter | Default | Purpose |
|-----------|---------|---------|
| `positionSmoothTime` | 0.8s | Position interpolation |
| `rotationSmoothTime` | 0.5s | Look-at interpolation |
| `fovSmoothTime` | 1.5s | FOV transitions |

Lower values = snappier response, higher values = smoother but more delayed.

### Collision Avoidance Pipeline

1. Calculate desired camera position
2. Check against terrain heightmap + padding
3. Check against water level + margin
4. Return corrected position

---

## Performance Considerations

- Collision avoidance uses simple heightmap lookups (O(1))
- Target selection iterates creature pool only on switch interval
- All smoothing uses lightweight spring calculations
- No allocations in update loop
- No additional draw calls or GPU resources required

---

## Usage Example

```cpp
#include "graphics/CameraController.h"
#include "entities/Creature.h"
#include "environment/Terrain.h"

// Get CameraController instance
Forge::CameraController& controller = getCameraController();

// Set terrain for collision avoidance
controller.setTerrain(terrain);
controller.setWaterLevel(terrain->getWaterLevel());

// Provide creature pool for auto-selection
controller.setCreaturePool(&activeCreatures);

// Configure cinematic settings
Forge::CinematicCameraConfig config = controller.getCinematicConfig();
config.preferredDistance = 40.0f;
config.orbitSpeed = 0.1f;
config.rollIntensity = 0.03f;
controller.setCinematicConfig(config);

// Configure target selection
Forge::TargetSelectionConfig targetConfig;
targetConfig.mode = Forge::TargetSelectionMode::LARGEST_CREATURE;
targetConfig.switchInterval = 20.0f;
controller.setTargetSelectionConfig(targetConfig);

// Register photo mode callback
controller.onPhotoModeFreeze = [](bool freeze) {
    setSimulationPaused(freeze);
};

// Start cinematic mode
controller.startFollowTarget(findInterestingCreature());

// In game loop
void update(float deltaTime) {
    controller.update(deltaTime);

    // Get roll for shader effects
    float roll = controller.getCinematicRoll();
    shader.setUniform("cameraRoll", roll);
}
```

---

## Future Enhancements

Potential improvements for future phases:
- Depth of field integration with photo mode focus distance
- Camera shake from nearby creature interactions
- Auto-framing with rule of thirds
- Cinematic path recording/playback
- Multiple camera presets with hotkeys
- Split-screen multi-creature view
