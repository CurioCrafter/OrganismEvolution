# Performance Optimization Fixes

This document describes the performance optimizations implemented to address issues 6.2, 6.3, and 6.4 from the Code Quality Audit.

## Summary

| Issue | Problem | Solution | Impact |
|-------|---------|----------|--------|
| 6.2 | Vector allocations every frame | Pre-allocated cached vectors | Eliminates ~2 heap allocations per frame |
| 6.3 | Redundant `glGetIntegerv(GL_VIEWPORT)` calls | Single viewport query per frame | Reduces OpenGL state queries by 4-5x |
| 6.4 | String operations with `std::stringstream` | `snprintf` into fixed buffer | Eliminates dynamic string allocation |

## Issue 6.2: Frame Vector Allocations

### Problem
In `updateCreatures()`, two vectors were being allocated every frame:
```cpp
// BEFORE - Allocates new vector every frame
std::vector<glm::vec3> foodPositions = getFoodPositions();
std::vector<Creature*> creaturePtrs;
```

This caused:
- 2 heap allocations per frame
- Memory fragmentation over time
- Cache misses from non-contiguous allocations

### Solution
Added pre-allocated member variables that are cleared and reused each frame:

**Simulation.h** - New member variables:
```cpp
// Performance optimization: Pre-allocated frame vectors (Issue 6.2)
mutable std::vector<glm::vec3> cachedFoodPositions;
mutable std::vector<Creature*> cachedCreaturePtrs;
```

**Simulation.cpp** - Constructor pre-allocation:
```cpp
// Pre-allocate vectors for performance (Issue 6.2)
cachedFoodPositions.reserve(256);
cachedCreaturePtrs.reserve(256);
```

**Simulation.cpp** - Usage in updateCreatures():
```cpp
// Clear without deallocating - maintains capacity
cachedFoodPositions.clear();
cachedCreaturePtrs.clear();
// ... populate vectors ...
```

### Impact
- **Before**: 2 heap allocations per frame (~0.5-2ms on some systems)
- **After**: 0 heap allocations in steady state
- **Estimated FPS gain**: 1-5 FPS depending on allocator performance

---

## Issue 6.3: Redundant Viewport Queries

### Problem
`glGetIntegerv(GL_VIEWPORT, viewport)` was called 5 times per frame:
1. `render()` - after shadow pass
2. `render()` - creature rendering section
3. `renderNametags()`
4. `renderVegetation()`
5. `renderDebugMarkers()`

Each call involves:
- Driver round-trip
- Potential GPU sync point
- Cache invalidation

### Solution
Query viewport once at the start of render and cache it for the frame:

**Simulation.h** - New member variables:
```cpp
// Performance optimization: Cached viewport (Issue 6.3)
mutable GLint cachedViewport[4];
mutable bool viewportCached;
```

**Simulation.cpp** - Single query in render():
```cpp
void Simulation::render(Camera& camera) {
    // Performance optimization (Issue 6.3): Cache viewport query once per frame
    glGetIntegerv(GL_VIEWPORT, cachedViewport);
    viewportCached = true;
    // ...
}
```

All render functions now use `cachedViewport` instead of querying:
```cpp
// BEFORE
GLint viewport[4];
glGetIntegerv(GL_VIEWPORT, viewport);
float aspectRatio = (viewport[3] > 0) ? (float)viewport[2] / (float)viewport[3] : 16.0f/9.0f;

// AFTER
float aspectRatio = (cachedViewport[3] > 0) ? (float)cachedViewport[2] / (float)cachedViewport[3] : 16.0f/9.0f;
```

### Files Modified
- `renderNametags()` - Uses cachedViewport
- `renderVegetation()` - Uses cachedViewport
- `renderDebugMarkers()` - Uses cachedViewport
- Creature rendering section in `render()` - Uses cachedViewport

### Impact
- **Before**: 5 OpenGL state queries per frame
- **After**: 1 OpenGL state query per frame
- **Reduction**: 80% fewer driver round-trips
- **Estimated FPS gain**: 0.5-2 FPS

---

## Issue 6.4: String Operations in Hot Path

### Problem
The stats display used `std::stringstream` every frame:
```cpp
// BEFORE - Creates stringstream object, performs dynamic allocation
std::stringstream stats;
stats << "Pop: " << std::setw(3) << simulation->getPopulation()
      << " (H:" << simulation->getHerbivoreCount()
      // ... etc
renderText(window, stats.str());
```

Issues:
- `std::stringstream` constructor allocates internal buffer
- Multiple `<<` operations may cause buffer reallocations
- `.str()` creates a copy of the string
- Total: 2-4 heap allocations per frame just for stats display

### Solution
1. Use fixed-size buffer with `snprintf`
2. Throttle output to once per second (was already throttled, but allocation still happened)

**main.cpp** - Fixed buffer:
```cpp
// Performance optimization (Issue 6.4): Fixed buffer for stats string
static char g_statsBuffer[256];
static float g_lastStatsPrint = 0.0f;

void renderText(GLFWwindow* window, const char* text) {
    if (totalTime - g_lastStatsPrint > 1.0f) {
        std::cout << "\r" << text << std::flush;
        g_lastStatsPrint = totalTime;
    }
}
```

**main.cpp** - snprintf usage:
```cpp
// Display stats (console) - Performance optimization (Issue 6.4)
snprintf(g_statsBuffer, sizeof(g_statsBuffer),
         "Pop: %3d (H:%d C:%d) | Food: %3d/%d | Gen: %3d | FPS: %3d",
         simulation->getPopulation(),
         simulation->getHerbivoreCount(),
         simulation->getCarnivoreCount(),
         simulation->getFoodCount(),
         simulation->getMaxFoodCount(),
         simulation->getGeneration(),
         (int)(1.0f / deltaTime));
renderText(window, g_statsBuffer);
```

### Impact
- **Before**: 2-4 heap allocations per frame for string formatting
- **After**: 0 heap allocations (static buffer)
- **Bonus**: `snprintf` is faster than iostream for formatted output
- **Estimated FPS gain**: 0.5-1 FPS

---

## Combined Impact

### Theoretical Performance Gains
| Optimization | Heap Allocations Saved | OpenGL Calls Saved | Est. FPS Gain |
|--------------|------------------------|-------------------|---------------|
| Issue 6.2    | ~2 per frame           | 0                 | 1-5 FPS       |
| Issue 6.3    | 0                      | 4 per frame       | 0.5-2 FPS     |
| Issue 6.4    | ~2-4 per frame         | 0                 | 0.5-1 FPS     |
| **Total**    | **~4-6 per frame**     | **4 per frame**   | **2-8 FPS**   |

### Verification Steps
1. **Build the project** and run the simulation
2. **Monitor FPS** - should see improvement especially with many creatures
3. **Use profiler** to verify:
   - No allocations in `updateCreatures()` (except initial reserve)
   - Single `glGetIntegerv` call per frame
   - No string allocations in main loop

### Future Optimizations
Consider these additional improvements:
1. **Object pooling** for creatures/food to avoid `make_unique` allocations
2. **Batch viewport calculation** into a struct passed to all render functions
3. **Frame statistics** update only when values change, not every frame
4. **Double-buffered vectors** for creature updates to enable threading

---

## Files Modified

| File | Changes |
|------|---------|
| `src/core/Simulation.h` | Added `cachedFoodPositions`, `cachedCreaturePtrs`, `cachedViewport`, `viewportCached` |
| `src/core/Simulation.cpp` | Constructor initialization, `updateCreatures()` rewrite, viewport caching in 5 functions |
| `src/main.cpp` | Added `g_statsBuffer`, converted `renderText()` to use `const char*`, replaced `stringstream` with `snprintf` |

---

*Document created: 2026-01-14*
*Related: CODE_QUALITY_AUDIT.md Issues 6.2, 6.3, 6.4*
