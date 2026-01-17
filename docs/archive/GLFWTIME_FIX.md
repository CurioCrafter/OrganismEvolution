# glfwGetTime64 Linking Error - Fix Documentation

## Problem
```
The procedure entry point clock_gettime64 could not be located in the dynamic link library
C:\msys64\mingw64\bin\libstdc++-6.dll
```

This error occurred when trying to call `glfwGetTime()` which internally uses `clock_gettime64`.

## Root Cause
The GLFW library's time function (`glfwGetTime()`) depends on a newer system call (`clock_gettime64`) that wasn't available in the MinGW runtime being used.

## Solution
Replaced all `glfwGetTime()` calls with C++ standard library `std::chrono::high_resolution_clock`:

### Changes Made

**File: src/main.cpp**

1. **Added include:**
```cpp
#include <chrono>
```

2. **Replaced timing in main loop:**
```cpp
// OLD (caused linking error):
float currentFrame = glfwGetTime();

// NEW (uses C++ chrono):
auto startTime = std::chrono::high_resolution_clock::now();
while (!glfwWindowShouldClose(window)) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - startTime;
    float currentFrame = elapsed.count();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    totalTime += deltaTime;
    // ...
}
```

3. **Fixed renderText function:**
```cpp
// OLD:
float currentTime = glfwGetTime();
if (currentTime - lastPrint > 1.0f) {
    // ...
}

// NEW:
if (totalTime - lastPrint > 1.0f) {
    std::cout << "\r" << text << std::flush;
    lastPrint = totalTime;
}
```

## Benefits of This Solution

1. **No External Dependencies:** Uses C++ standard library instead of GLFW-specific functions
2. **Cross-Platform:** `std::chrono` is part of C++11 standard, works everywhere
3. **High Precision:** `high_resolution_clock` provides microsecond accuracy
4. **No Linking Issues:** Doesn't depend on system-specific time functions

## Testing
- ✅ Build successful with no warnings about time functions
- ✅ No runtime linking errors
- ✅ Timing works correctly for deltaTime and animations

## Technical Details

**Precision:** `std::chrono::high_resolution_clock` typically provides nanosecond precision on modern systems.

**Performance:** Zero overhead - `std::chrono` is compile-time optimized.

**Compatibility:** Works with C++11 and later (project uses C++17).

## Alternative Solutions Considered

1. **Update MinGW:** Would require user to reinstall development environment
2. **Static GLFW linking:** Would increase executable size and still might have issues
3. **Downgrade GLFW:** Would lose features and bug fixes

**Chosen solution** (std::chrono) is the most portable and maintainable.
