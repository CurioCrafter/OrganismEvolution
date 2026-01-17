# Testing Checklist - OrganismEvolution DirectX 12

**Version:** Phase 2 Complete
**Date:** January 14, 2026

This checklist provides step-by-step testing procedures for all major features of the simulation.

---

## Pre-Testing Setup

### Build Verification
- [ ] Project compiles without errors
- [ ] All 18 HLSL shaders present in `shaders/hlsl/`
- [ ] Executable runs without immediate crash
- [ ] Console window shows initialization messages

### Expected Startup Output
```
Initializing DirectX 12 renderer...
  Device created successfully
  Swapchain created successfully
  Depth buffer created successfully
  Constant buffers created (2 per-frame)
  Frame fence created (initial frame index: 0)
```

---

## 1. Water Rendering Tests

### Test 1.1: Water Level
**Steps:**
1. Launch the application
2. Move camera to edge of terrain where water meets land
3. Observe water coverage

**Expected Results:**
- [ ] Water covers only lowest areas of terrain (normalized height < 0.02)
- [ ] Approximately 60% of terrain is above water
- [ ] Clear distinction between water and beach biome

### Test 1.2: Wave Animation
**Steps:**
1. Position camera looking at water surface
2. Observe wave motion for 10-15 seconds

**Expected Results:**
- [ ] Waves appear gentle and calm (not choppy)
- [ ] Wave speed is moderate (not too fast)
- [ ] Wave height creates subtle ripples (not dramatic displacement)

### Test 1.3: Water-Beach Transition
**Steps:**
1. Move camera along shoreline
2. Observe color transition from water to beach to grass

**Expected Results:**
- [ ] Smooth gradient from water blue to beach tan
- [ ] Beach zone starts immediately above water level
- [ ] No harsh color boundaries

---

## 2. Creature Visibility Tests

### Test 2.1: Creature Spawning
**Steps:**
1. Launch application
2. Check console for creature spawn messages
3. Move camera to terrain center

**Expected Results:**
- [ ] Console shows "Spawned X herbivores, Y carnivores"
- [ ] Creatures visible on terrain (Note: Currently DEBUG mode shows magenta)
- [ ] Different colors for herbivores vs carnivores (when DEBUG disabled)

### Test 2.2: Creature Movement
**Steps:**
1. Observe creatures for 30 seconds
2. Watch movement patterns

**Expected Results:**
- [ ] Creatures move smoothly across terrain
- [ ] Herbivores exhibit flocking behavior
- [ ] Carnivores pursue herbivores
- [ ] No creatures stuck in place

### Test 2.3: Creature Population
**Steps:**
1. Watch population counter in console output
2. Let simulation run for 2-3 minutes

**Expected Results:**
- [ ] Population fluctuates naturally
- [ ] Herbivore count: 10-150 range
- [ ] Carnivore count: 3-30 range
- [ ] Auto-replenishment when population drops too low

---

## 3. Camera Control Tests

### Test 3.1: WASD Movement
**Steps:**
1. Press and hold W key - camera should move forward
2. Press and hold S key - camera should move backward
3. Press and hold A key - camera should strafe left
4. Press and hold D key - camera should strafe right

**Expected Results:**
- [ ] Smooth movement in all directions
- [ ] No stuttering or jumping
- [ ] Movement speed feels natural (50 units/sec base)

### Test 3.2: Vertical Movement
**Steps:**
1. Press and hold Space - camera should rise
2. Press and hold C or Ctrl - camera should lower
3. Press and hold Q - camera should lower
4. Press and hold E - camera should rise

**Expected Results:**
- [ ] Smooth vertical movement
- [ ] All vertical controls work as expected
- [ ] No clipping through terrain

### Test 3.3: Mouse Look
**Steps:**
1. Click left mouse button to capture cursor
2. Move mouse left/right - camera should pan horizontally
3. Move mouse up/down - camera should tilt vertically
4. Press Escape to release cursor

**Expected Results:**
- [ ] Cursor captured and hidden on click
- [ ] Smooth camera rotation following mouse
- [ ] Pitch clamped to prevent over-rotation (+-1.5 radians)
- [ ] Cursor released and visible after Escape

### Test 3.4: Camera Smoothing
**Steps:**
1. Make rapid camera movements
2. Stop all input suddenly
3. Observe camera behavior

**Expected Results:**
- [ ] Camera movement has slight smoothing
- [ ] Camera stops completely when input stops (no drift)
- [ ] Smoothing reduces jitter without adding latency

### Test 3.5: Sprint Speed
**Steps:**
1. Hold Shift + W to sprint forward

**Expected Results:**
- [ ] Camera moves at 2x normal speed
- [ ] Works with all movement directions

---

## 4. ImGui Interaction Tests

### Test 4.1: Debug Panel Toggle
**Steps:**
1. Press F3 key
2. Observe screen for ImGui panel
3. Press F3 again

**Expected Results:**
- [ ] F3 toggles debug panel visibility
- [ ] Panel appears/disappears cleanly
- [ ] No rendering artifacts

### Test 4.2: Debug Panel Contents
**Steps:**
1. Open debug panel (F3)
2. Review displayed information

**Expected Results:**
- [ ] FPS counter displayed
- [ ] Population statistics shown
- [ ] Simulation speed control available
- [ ] Pause button functional

### Test 4.3: Mouse Interaction with ImGui
**Steps:**
1. Open debug panel
2. Try to interact with panel elements
3. Note cursor capture state

**Expected Results:**
- [ ] Can interact with ImGui when panel is open
- [ ] Mouse look disabled when interacting with panel
- [ ] Proper input state management

---

## 5. Nametag Tests

### Test 5.1: Nametag Visibility
**Steps:**
1. Move camera close to creatures
2. Look for floating ID tags above creatures

**Expected Results:**
- [ ] Nametags visible above each creature
- [ ] ID numbers displayed correctly
- [ ] Type indicator shown (H or C)

### Test 5.2: Nametag Billboarding
**Steps:**
1. Move camera around a creature
2. Observe nametag orientation

**Expected Results:**
- [ ] Nametag always faces camera
- [ ] Text remains readable from all angles
- [ ] Semi-transparent background visible

---

## 6. Lighting and Day/Night Tests

### Test 6.1: Lighting Stability
**Steps:**
1. Run simulation for 5+ minutes
2. Observe lighting on terrain and creatures

**Expected Results:**
- [ ] No flickering or sudden brightness changes
- [ ] Smooth lighting transitions
- [ ] Terrain has visible biome colors

### Test 6.2: Day/Night Cycle
**Steps:**
1. Watch sky colors over time
2. Observe lighting intensity changes

**Expected Results:**
- [ ] Sky colors transition smoothly
- [ ] Dawn and dusk have golden hour colors
- [ ] Night maintains minimum visibility (not completely dark)
- [ ] Stars visible at night (if enabled)

### Test 6.3: Ambient Lighting
**Steps:**
1. Position camera in shadowed area
2. Observe terrain visibility

**Expected Results:**
- [ ] Shadowed areas still visible
- [ ] Ambient light provides base illumination
- [ ] No completely black areas on terrain

---

## 7. Performance Tests

### Test 7.1: FPS Stability
**Steps:**
1. Monitor FPS counter for 5 minutes
2. Record minimum and maximum FPS

**Expected Results:**
- [ ] Stable 60 FPS (with vsync)
- [ ] No major frame drops
- [ ] No memory leaks (stable RAM usage)

### Test 7.2: Population Scaling
**Steps:**
1. Let population grow to maximum
2. Monitor FPS during peak population

**Expected Results:**
- [ ] Maintains playable FPS with 100+ creatures
- [ ] Instanced rendering reduces draw calls
- [ ] No significant slowdown

---

## 8. Simulation Logic Tests

### Test 8.1: Pause/Resume
**Steps:**
1. Press P to pause simulation
2. Observe creatures stop moving
3. Press P to resume

**Expected Results:**
- [ ] P key toggles pause state
- [ ] Creatures freeze when paused
- [ ] Time/animation continues (camera still works)
- [ ] Simulation resumes correctly

### Test 8.2: Speed Control
**Steps:**
1. Use speed slider in debug panel
2. Adjust simulation speed

**Expected Results:**
- [ ] Speed changes apply immediately
- [ ] Creatures move faster/slower accordingly
- [ ] Console output reflects current speed

### Test 8.3: Evolution Progression
**Steps:**
1. Run simulation for 10+ minutes
2. Monitor generation counter

**Expected Results:**
- [ ] Generation counter increases over time
- [ ] New creatures appear via reproduction
- [ ] Population maintains balance

---

## Post-Testing Cleanup

### Shutdown
- [ ] Press Escape or close window
- [ ] Application exits cleanly
- [ ] No error messages on exit
- [ ] GPU resources released (no device lost)

---

## Issue Reporting Template

If any test fails, document using this template:

```
Test ID: [e.g., 3.3]
Test Name: [e.g., Mouse Look]
Status: FAILED
Description: [What happened]
Expected: [What should have happened]
Steps to Reproduce:
1. ...
2. ...
System Info:
- GPU: [model]
- Driver: [version]
- Windows: [version]
Screenshots: [if applicable]
```

---

*Testing Checklist - Version 1.0*
*OrganismEvolution DirectX 12 - January 2026*
