# Professional Game Loop Patterns

## Overview

This document covers professional game loop implementation patterns, comparing our current implementation against industry best practices. The game loop is the heartbeat of any real-time simulation - it must handle variable hardware performance while maintaining deterministic gameplay.

## Game Loop Variants

### 1. Simple Variable Timestep (NOT Recommended)

```cpp
// PROBLEMATIC - Physics depends on frame rate
while (running) {
    double current = getCurrentTime();
    double elapsed = current - lastTime;
    lastTime = current;

    processInput();
    update(elapsed);  // Variable step - non-deterministic!
    render();
}
```

**Problems:**
- Physics simulations become unstable at low frame rates
- Different machines produce different gameplay results
- Floating-point errors accumulate differently
- Networked games diverge between clients

### 2. Fixed Timestep with Sleep (Basic)

```cpp
// Better but inflexible
const double MS_PER_FRAME = 16.67;  // 60 FPS target

while (running) {
    double start = getCurrentTime();

    processInput();
    update();
    render();

    double elapsed = getCurrentTime() - start;
    if (elapsed < MS_PER_FRAME) {
        sleep(MS_PER_FRAME - elapsed);
    }
}
```

**Problems:**
- Cannot recover if frame takes too long
- Wastes CPU when frames finish early
- Sleep is imprecise - OS thread scheduler introduces jitter

### 3. Fixed Update, Variable Render (Industry Standard)

```cpp
// RECOMMENDED - Used by professional game engines
double previous = getCurrentTime();
double lag = 0.0;
const double MS_PER_UPDATE = 16.0;  // 60 physics ticks/sec

while (running) {
    double current = getCurrentTime();
    double elapsed = current - previous;
    previous = current;
    lag += elapsed;

    processInput();

    // Fixed timestep physics updates
    while (lag >= MS_PER_UPDATE) {
        update(MS_PER_UPDATE);
        lag -= MS_PER_UPDATE;
    }

    // Variable rate rendering with interpolation
    render(lag / MS_PER_UPDATE);
}
```

**Benefits:**
- Deterministic physics - same input = same output
- Rendering adapts to hardware capability
- Physics cannot "explode" from large timesteps
- Multiplayer-ready - all clients simulate identically

## Our Current Implementation Analysis

### main.cpp Game Loop

```cpp
// Current implementation (simplified)
auto startTime = std::chrono::high_resolution_clock::now();
while (!glfwWindowShouldClose(window)) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - startTime;
    float currentFrame = elapsed.count();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Delta capping (GOOD practice)
    const float MAX_DELTA_TIME = 0.1f;
    if (deltaTime > MAX_DELTA_TIME) {
        deltaTime = MAX_DELTA_TIME;
    }

    processInput(window);
    simulation->update(deltaTime);  // Variable timestep

    // Render phase
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    simulation->render(camera);
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

### Current Strengths

1. **Delta Time Capping** - Prevents physics explosion after alt-tab
2. **Chrono-based Timing** - More precise than glfwGetTime
3. **Focus-Aware Input** - Handles window focus loss gracefully
4. **Speed Multiplier** - `simulationSpeed` scales delta for fast-forward

### Current Weaknesses

1. **Variable Timestep Physics** - Creatures update with variable delta
2. **No Interpolation** - Rendering shows discrete physics positions
3. **Coupled Update/Render** - Can't run physics faster than rendering
4. **No Fixed Accumulator** - Physics rate tied to frame rate

## Recommended Implementation

### Fixed Timestep with Interpolation

```cpp
// Improved game loop structure
class GameLoop {
private:
    static constexpr double PHYSICS_TIMESTEP = 1.0 / 60.0;  // 60 Hz physics
    static constexpr int MAX_PHYSICS_STEPS = 5;  // Prevent spiral of death

    double accumulator = 0.0;
    double previousTime = 0.0;

public:
    void run() {
        previousTime = getCurrentTime();

        while (running) {
            double currentTime = getCurrentTime();
            double frameTime = currentTime - previousTime;
            previousTime = currentTime;

            // Cap frame time to prevent spiral of death
            if (frameTime > 0.25) {
                frameTime = 0.25;
            }

            accumulator += frameTime;

            // Process input once per frame
            processInput();

            // Fixed timestep physics updates
            int physicsSteps = 0;
            while (accumulator >= PHYSICS_TIMESTEP && physicsSteps < MAX_PHYSICS_STEPS) {
                fixedUpdate(PHYSICS_TIMESTEP);
                accumulator -= PHYSICS_TIMESTEP;
                physicsSteps++;
            }

            // Calculate interpolation factor for smooth rendering
            double alpha = accumulator / PHYSICS_TIMESTEP;

            // Render with interpolated positions
            render(alpha);

            swapBuffers();
        }
    }

    void fixedUpdate(double dt) {
        // Physics, AI, game logic - always same timestep
        simulation->physicsUpdate(dt);
        simulation->aiUpdate(dt);
        simulation->collisionDetection();
    }

    void render(double alpha) {
        // Interpolate visual positions between physics states
        // previousState + (currentState - previousState) * alpha
        simulation->render(camera, alpha);
    }
};
```

### Interpolation Implementation

```cpp
// Creature state interpolation
struct CreatureRenderState {
    glm::vec3 previousPosition;
    glm::vec3 currentPosition;
    float previousRotation;
    float currentRotation;

    glm::vec3 getInterpolatedPosition(float alpha) const {
        return previousPosition + (currentPosition - previousPosition) * alpha;
    }

    float getInterpolatedRotation(float alpha) const {
        // Handle angle wrapping for smooth rotation
        float diff = currentRotation - previousRotation;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;
        return previousRotation + diff * alpha;
    }
};

// In Creature::update()
void Creature::prePhysicsUpdate() {
    renderState.previousPosition = renderState.currentPosition;
    renderState.previousRotation = renderState.currentRotation;
}

void Creature::postPhysicsUpdate() {
    renderState.currentPosition = position;
    renderState.currentRotation = rotation;
}
```

## Frame Pacing Strategies

### V-Sync Considerations

```cpp
// V-Sync ON (60 Hz display)
// - Prevents tearing
// - Caps frame rate to refresh rate
// - Can cause input lag (up to 16.67ms)

glfwSwapInterval(1);  // Enable V-Sync

// V-Sync OFF with frame limiting
// - Allows higher frame rates
// - May cause screen tearing
// - Lower input latency

glfwSwapInterval(0);  // Disable V-Sync

// Manual frame limiting (for consistent timing)
void limitFrameRate(double targetFPS) {
    double targetFrameTime = 1.0 / targetFPS;
    double elapsed = getCurrentTime() - frameStart;

    if (elapsed < targetFrameTime) {
        // Busy-wait for remaining time (more precise than sleep)
        while (getCurrentTime() - frameStart < targetFrameTime) {
            // Spin
        }
    }
}
```

### Triple Buffering

```cpp
// Triple buffering reduces stutter with V-Sync
// Front buffer: Currently displayed
// Back buffer: Being rendered to
// Third buffer: Waiting to be displayed

// GLFW doesn't expose this directly - driver controlled
// Can hint via window creation:
glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
```

## Input Lag Mitigation

### Input Sampling Strategy

```cpp
// Sample input as late as possible before rendering
void gameLoop() {
    while (running) {
        // ... physics accumulator logic ...

        // Sample input AFTER physics, BEFORE render
        // This minimizes input-to-display latency
        processInput();

        render(alpha);
        swapBuffers();
    }
}
```

### Predictive Input

```cpp
// For competitive games, predict input during render
struct InputState {
    glm::vec3 predictedCameraPosition;
    float inputTimestamp;

    void predict(float renderTime) {
        // Extrapolate camera based on current velocity
        float dt = renderTime - inputTimestamp;
        predictedCameraPosition = currentPosition + velocity * dt;
    }
};
```

## Render/Update Separation

### Decoupled Systems

```cpp
class SimulationSystems {
public:
    // Fixed timestep systems (order matters)
    void fixedUpdate(double dt) {
        environmentSystem.update(dt);      // Weather, day/night
        spatialSystem.rebuild();           // Spatial grid
        aiSystem.update(dt);               // Creature decisions
        physicsSystem.update(dt);          // Movement, collision
        reproductionSystem.update(dt);     // Population dynamics
        audioSystem.update(dt);            // Sound propagation
    }

    // Variable timestep systems
    void variableUpdate(double dt) {
        animationSystem.update(dt);        // Procedural animation
        particleSystem.update(dt);         // Visual effects
    }

    // Render systems (no gameplay logic)
    void render(double alpha) {
        shadowSystem.render();             // Shadow pass
        terrainRenderer.render();          // Ground
        vegetationRenderer.render();       // Trees/grass
        creatureRenderer.render(alpha);    // Interpolated creatures
        effectRenderer.render();           // Particles
        uiRenderer.render();               // HUD/debug
    }
};
```

## Code Examples for Migration

### Step 1: Add State Buffering to Creature

```cpp
// In Creature.h
struct PhysicsState {
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;
};

class Creature {
    PhysicsState previousState;
    PhysicsState currentState;

    void saveState() {
        previousState = currentState;
    }

    glm::vec3 getInterpolatedPosition(float alpha) const {
        return glm::mix(previousState.position, currentState.position, alpha);
    }
};
```

### Step 2: Separate Physics from Rendering

```cpp
// In Simulation.cpp
void Simulation::fixedUpdate(float dt) {
    // Save previous state for interpolation
    for (auto& creature : creatures) {
        creature->saveState();
    }

    // Fixed timestep updates
    updateEnvironmentConditions(dt);
    rebuildSpatialGrid();
    updateCreaturesPhysics(dt);
    handleReproduction();
    balancePopulation();
}

void Simulation::render(Camera& camera, float alpha) {
    renderShadowPass();
    renderTerrain();
    renderVegetation(camera);

    // Pass interpolation alpha to creature renderer
    creatureRenderer->render(creatures, camera, *creatureShader, alpha);

    renderNametags(camera);
}
```

### Step 3: Update Main Loop

```cpp
// In main.cpp
const double FIXED_TIMESTEP = 1.0 / 60.0;
double accumulator = 0.0;

while (!glfwWindowShouldClose(window)) {
    double frameTime = calculateDeltaTime();
    frameTime = std::min(frameTime, 0.25);  // Cap
    accumulator += frameTime;

    processInput(window);

    // Fixed timestep updates
    while (accumulator >= FIXED_TIMESTEP) {
        simulation->fixedUpdate(FIXED_TIMESTEP);
        accumulator -= FIXED_TIMESTEP;
    }

    // Calculate interpolation factor
    float alpha = static_cast<float>(accumulator / FIXED_TIMESTEP);

    // Render with interpolation
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    simulation->render(camera, alpha);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

## Performance Profiling

### Measuring Frame Times

```cpp
class FrameProfiler {
    std::array<double, 120> frameTimes;
    int frameIndex = 0;

public:
    void recordFrame(double time) {
        frameTimes[frameIndex] = time;
        frameIndex = (frameIndex + 1) % frameTimes.size();
    }

    double getAverageFrameTime() const {
        return std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0)
               / frameTimes.size();
    }

    double get99thPercentile() const {
        auto sorted = frameTimes;
        std::sort(sorted.begin(), sorted.end());
        return sorted[static_cast<int>(sorted.size() * 0.99)];
    }
};
```

## External References

### Essential Reading

- [Game Loop Pattern - Game Programming Patterns](https://gameprogrammingpatterns.com/game-loop.html)
- [Fix Your Timestep! - Glenn Fiedler](https://gafferongames.com/post/fix_your_timestep/)
- [Taming Time in Game Engines - André Leite](https://andreleite.com/posts/2025/game-loop/fixed-timestep-game-loop/)
- [Update Method Pattern - Game Programming Patterns](https://gameprogrammingpatterns.com/update-method.html)

### GDC Talks

- "Math for Game Programmers: Interaction with 3D Geometry"
- "Parallelizing the Naughty Dog Engine"
- "Destiny's Multithreaded Rendering Architecture"

### Books

- *Game Programming Patterns* by Robert Nystrom
- *Game Engine Architecture* by Jason Gregory
- *Real-Time Rendering* by Tomas Akenine-Möller

## Summary

| Pattern | Determinism | Smoothness | Complexity | Recommended |
|---------|-------------|------------|------------|-------------|
| Variable timestep | Poor | Moderate | Low | No |
| Fixed with sleep | Good | Poor | Low | No |
| Fixed + interpolation | Excellent | Excellent | Moderate | **Yes** |
| Multi-threaded | Excellent | Excellent | High | Advanced |

The **fixed timestep with variable rendering and interpolation** pattern is the industry standard for professional games and simulations. It provides deterministic physics, smooth visuals at any frame rate, and a solid foundation for multiplayer synchronization.
