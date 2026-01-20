# Controls Reference - OrganismEvolution DirectX 12

**Version:** Phase 11 Current
**Date:** January 19, 2026

This document provides a complete reference for all keyboard shortcuts, mouse controls, and UI interactions.

---

## Quick Reference Card

```
MOVEMENT                    CAMERA                      SIMULATION
--------------------------------------------------------------------------------------------------------
W     - Forward             Mouse Move  - Look around   P         - Pause/Resume
S     - Backward            Left Click  - Capture       F1        - Toggle Debug Panel
A     - Strafe Left         Escape      - Release       F2        - Toggle Performance Profiler
D     - Strafe Right        Mouse Wheel - Zoom          F3        - Toggle Help Overlay
Space - Up                                              F5        - Quick Save
C     - Down                                            F9        - Quick Load
Ctrl  - Down (alt)                                      F10       - Toggle Replay Mode
Q     - Down (alt)                                      1-6       - Set Simulation Speed
E     - Up (alt)
Shift - Sprint (2x speed)
```

---

## Keyboard Controls

### Movement Keys

| Key | Action | Details |
|-----|--------|---------|
| **W** | Move Forward | Moves camera in the direction it's facing |
| **S** | Move Backward | Moves camera opposite to facing direction |
| **A** | Strafe Left | Moves camera left (perpendicular to facing) |
| **D** | Strafe Right | Moves camera right (perpendicular to facing) |
| **Space** | Move Up | Raises camera vertically |
| **C** | Move Down | Lowers camera vertically |
| **Ctrl** | Move Down | Alternative key for lowering camera |
| **Q** | Move Down | Alternative key for lowering camera |
| **E** | Move Up | Alternative key for raising camera |

### Movement Modifiers

| Key | Action | Details |
|-----|--------|---------|
| **Shift** | Sprint | Hold with movement keys for 2x speed |

### Base Speed
- Normal movement: 50 units/second
- Sprint movement: 100 units/second

---

### Simulation Control Keys

| Key | Action | Details |
|-----|--------|---------|
| **P** | Pause/Resume | Toggles simulation pause state |
| **F1** | Debug Panel | Toggles ImGui debug panel visibility |
| **F2** | Performance Profiler | Toggles performance overlay |
| **F3** | Help Overlay | Toggles help window with controls |
| **F5** | Quick Save | Saves current simulation state |
| **F9** | Quick Load | Loads last quick save |
| **F10** | Replay Mode | Toggles replay recording/playback |
| **Escape** | Release Mouse | Releases captured mouse cursor |

### Speed Controls

| Key | Action | Details |
|-----|--------|---------|
| **1** | 0.25x Speed | Quarter speed |
| **2** | 0.5x Speed | Half speed |
| **3** | 1x Speed | Normal speed |
| **4** | 2x Speed | Double speed |
| **5** | 4x Speed | Quadruple speed |
| **6** | 8x Speed | Maximum speed |

Note: Speed control is also available through the ImGui debug panel (F1).

---

## Mouse Controls

### Camera Look

| Action | Effect |
|--------|--------|
| **Left Click** (on window) | Capture mouse cursor for camera look |
| **Mouse Move** (when captured) | Rotate camera view |
| **Escape** | Release mouse cursor |

### Camera Look Details

- **Sensitivity:** 0.003 (configurable in code)
- **Horizontal:** Move mouse left/right to pan camera
- **Vertical:** Move mouse up/down to tilt camera
- **Pitch Limit:** +/- 1.5 radians (~86 degrees) to prevent over-rotation
- **Smoothing:** Applied to reduce jitter

### Mouse Capture Behavior

1. **Click** anywhere in the game window to capture cursor
2. Cursor becomes **hidden** and **centered**
3. Mouse movements rotate camera view
4. Press **Escape** to release cursor

### ImGui Interaction

When debug panel is open:
- Mouse can interact with panel elements
- Click outside panel to re-capture for camera look
- Buttons, sliders, and checkboxes respond to clicks

---

## ImGui Debug Panel

### Accessing the Panel

Press **F1** to toggle the debug panel visibility.

### Panel Contents

#### Statistics Display
- **FPS**: Current frames per second
- **Frame Time**: Milliseconds per frame
- **Population**: Total creature count by type
- **Food**: Active food item count
- **Time**: Simulation time and day/night cycle

#### Simulation Controls
- **Pause Button**: Toggle simulation pause
- **Speed Slider**: Adjust simulation speed (0.25x to 8x)

#### Creature Controls
- **Spawn Buttons**: Add creatures of specific types
- **Kill All Button**: Remove all creatures
- **Population Limits**: Set min/max creature counts

#### Camera Controls
- **Camera Mode**: Switch between Free, Orbit, Follow, Cinematic
- **Focus/Track**: Select and follow creatures

#### Save/Load Controls
- **Quick Save/Load**: F5/F9 buttons
- **Save Slots**: Multiple save slot selection
- **Auto-Save**: Toggle automatic saving

#### Replay Controls
- **Record/Play**: Toggle replay mode
- **Playback Controls**: Frame stepping, speed adjustment

---

## Camera System

### View Matrix

The camera uses a free-look FPS-style system:
- **Position**: 3D world coordinates (X, Y, Z)
- **Pitch**: Vertical angle (radians)
- **Yaw**: Horizontal angle (radians)
- **Zoom/FOV**: Field of view (45 degrees default)

### Default Camera Position

On startup:
```
Position: (0, 100, 200)
Pitch: -0.4 radians (looking slightly down)
Yaw: pi radians (facing toward terrain center)
```

### Camera Smoothing

Camera uses exponential smoothing for smoother rendering:
- Smooth factor: 0.15
- Position threshold: 0.001 units
- Rotation threshold: 0.0001 radians
- Smoothing stops when camera converges to target

---

## Control Schemes Comparison

### FPS-Style (Current)

```
         W
         ^
    A <- + -> D
         v
         S

+ Space/E = Up
+ C/Q/Ctrl = Down
+ Shift = Sprint
+ Mouse = Look
```

### Alternative Schemes (Not Implemented)

Potential future options:
- **Orbit Mode**: Click-drag to orbit around a point
- **Follow Mode**: Camera follows selected creature
- **Top-Down Mode**: Bird's eye view with pan/zoom

---

## Gamepad Support

**Status:** Not currently implemented

Future gamepad mapping (planned):
- Left Stick: Movement
- Right Stick: Camera look
- L3: Sprint toggle
- Start: Pause
- Select: Debug panel

---

## Accessibility Notes

### Key Remapping

Currently no in-game key remapping. To change controls:
1. Modify `ProcessInput()` function in `src/main.cpp`
2. Change `GetAsyncKeyState()` key codes
3. Rebuild application

### Mouse Sensitivity

To adjust mouse sensitivity:
1. Open `src/main.cpp`
2. Find line: `f32 sensitivity = 0.003f;`
3. Increase for faster, decrease for slower
4. Rebuild application

### Invert Y-Axis

To invert mouse Y-axis (pitch):
1. Open `src/main.cpp`
2. Find line: `m_camera.pitch -= s_accumulatedDY * sensitivity;`
3. Change `-=` to `+=`
4. Rebuild application

---

## Troubleshooting Controls

### Mouse Look Not Working

1. **Check cursor capture**: Click in window to capture
2. **Check debug panel**: Close if open (F3)
3. **Check focus**: Ensure game window has focus

### Movement Keys Not Responding

1. **Check pause state**: Press P to unpause
2. **Check focus**: Click in game window
3. **Check keyboard layout**: Ensure QWERTY layout

### Camera Drift

If camera slowly drifts when not moving:
1. This issue has been fixed in Phase 2
2. If still occurring, check for stuck keys
3. Report as bug if persistent

### Controls Feel Sluggish

1. Check FPS - low FPS causes slow input response
2. Close other applications to free GPU resources
3. Reduce simulation speed if CPU-bound

---

## Key Code Reference

For developers modifying controls:

| Key | VK Code | Hex |
|-----|---------|-----|
| W | 0x57 | 'W' |
| A | 0x41 | 'A' |
| S | 0x53 | 'S' |
| D | 0x44 | 'D' |
| Space | VK_SPACE | 0x20 |
| Shift | VK_SHIFT | 0x10 |
| Ctrl | VK_CONTROL | 0x11 |
| Escape | VK_ESCAPE | 0x1B |
| F3 | VK_F3 | 0x72 |
| P | 0x50 | 'P' |
| Q | 0x51 | 'Q' |
| E | 0x45 | 'E' |
| C | 0x43 | 'C' |

---

*Controls Reference - Version 2.0*
*OrganismEvolution DirectX 12 - January 19, 2026*
