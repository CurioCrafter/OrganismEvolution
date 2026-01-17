# Win32 Window Input Handling - Research Summary

## Executive Summary

This research package contains comprehensive information about Win32 window input handling, covering the fundamental mechanisms that power input processing in Windows applications. While your OrganismEvolution project uses GLFW (which abstracts Win32 complexity), understanding these core concepts helps diagnose input issues.

**Key Finding:** Win32 input requires a functional message pump calling `TranslateMessage()` and `DispatchMessage()`. GLFW handles this internally, but you must call `glfwPollEvents()` every frame.

---

## Document Overview

### 1. WIN32_INPUT_DIAGNOSTICS.md
**Comprehensive technical reference** covering:
- Common reasons windows don't receive input
- Window styles affecting input (WS_VISIBLE, WS_DISABLED, WS_EX_TRANSPARENT)
- Focus requirements and management
- Message pump problems and solutions
- GetAsyncKeyState vs WM_KEYDOWN differences
- Debug techniques and tools
- Complete verification checklist

**Best for:** Understanding the "why" behind input issues and deep technical troubleshooting

### 2. WIN32_INPUT_CHECKLIST.md
**Quick-reference diagnostic checklist** with:
- Critical issues to check first
- Window creation requirements
- Message pump verification procedures
- GLFW-specific checks
- Input reception verification
- Focus management checklist
- Common problems and solutions table
- Minimal code examples for debugging

**Best for:** Rapidly diagnosing input problems with a structured approach

### 3. WIN32_INPUT_CODE_EXAMPLES.md
**Practical, runnable code samples** including:
- Basic message pump implementations (blocking and non-blocking)
- Window procedure input handlers (keyboard, mouse, focus)
- Input state tracking classes
- GetAsyncKeyState polling patterns
- Combo detection system
- Raw input handling (advanced)
- GLFW input examples (polling and event-based)
- Diagnostic code for troubleshooting

**Best for:** Copy-paste solutions and understanding implementation patterns

---

## Core Concepts

### Win32 Message Pump (The Foundation)

Every Windows application has a message pump that processes input:

```cpp
// Blocking pattern (typical for UI apps)
MSG msg = {};
while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);  // Virtual key -> character
    DispatchMessage(&msg);   // Send to window procedure
}

// Non-blocking pattern (typical for games)
MSG msg = {};
while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
```

**Critical Line:** `TranslateMessage()` must be called before `DispatchMessage()` or text input won't work.

### Window Styles for Input

```cpp
// REQUIRED for input:
WS_OVERLAPPEDWINDOW  // Base styles for normal windows
WS_VISIBLE           // Window must be visible to receive input

// FATAL for input:
WS_EX_TRANSPARENT    // Blocks ALL mouse input
WS_EX_NOACTIVATE     // Can't receive focus
WS_DISABLED          // Can't receive any input
```

### Your Project: GLFW Abstraction

```cpp
// Your main loop (from src/main.cpp)
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();        // <-- This calls TranslateMessage/DispatchMessage internally
    processInput(window);    // <-- Your polling function (continuous input)
    // ... render ...
    glfwSwapBuffers(window);
}

// Your input processing
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)  // Polling
        camera.processKeyboard(FORWARD, deltaTime);
}

// Your callbacks
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Event-driven
    camera.processMouseMovement(xoffset, yoffset);
}
```

GLFW provides both approaches:
- **Polling** via `glfwGetKey()` - for continuous input
- **Callbacks** via `glfwSetCursorPosCallback()` - for events

---

## Input Methods Comparison

| Aspect | GetAsyncKeyState (Polling) | WM_KEYDOWN (Events) | GLFW glfwGetKey | GLFW Callbacks |
|--------|---------------------------|-------------------|-----------------|----------------|
| **Use Case** | Continuous movement | One-time actions | Continuous input | Event-driven |
| **Latency** | Very low | Slight delay | Very low | Low |
| **Works Unfocused** | YES (dangerous) | NO | NO | NO |
| **Auto-repeat** | N/A | YES (confusing) | N/A | N/A |
| **Text Input** | NO | YES (via WM_CHAR) | NO | YES (callbacks) |
| **Easy to Use** | YES | Complex | YES | Very good |
| **Recommended for** | WASD camera control | UI buttons, firing | WASD movement | Mouse movement |

---

## Your Project Assessment

### Current Implementation (Strengths)

1. **GLFW is correctly initialized**
   - Window created successfully
   - Context properly set with `glfwMakeContextCurrent()`
   - All callbacks registered (`glfwSetCursorPosCallback`, etc.)

2. **Message loop is correct** (abstracted by GLFW)
   - `glfwPollEvents()` called every frame
   - `glfwSwapBuffers()` called after rendering
   - Proper event ordering maintained

3. **Input processing is well-designed**
   - Polling for continuous input (WASD movement)
   - State tracking for discrete actions (P key for pause)
   - Separate handlers for different input types

4. **Focus management is handled**
   - Window has focus after creation
   - Mouse capture toggles properly with TAB

### Potential Issues (If You Experience Input Problems)

1. **glfwPollEvents() not called**
   - Symptom: No mouse movement callback, keyboard input doesn't work
   - Fix: Ensure it's in the main loop before rendering
   - Status in your code: ✓ Correctly implemented

2. **Window lost focus**
   - Symptom: Keyboard input stops, mouse doesn't move when captured
   - Fix: Handle WM_KILLFOCUS/WM_SETFOCUS (GLFW does this)
   - Status in your code: ✓ GLFW handles this

3. **Callback not registered**
   - Symptom: Mouse movement doesn't work, scroll doesn't zoom
   - Fix: Verify `glfwSetCursorPosCallback` and other calls
   - Status in your code: ✓ All callbacks registered

4. **Blocking operation in callback**
   - Symptom: Input lag, frame rate drops when typing
   - Fix: Move heavy work out of callbacks
   - Status in your code: ✓ Callbacks are lightweight

5. **Rapid glfwPollEvents() calls**
   - Symptom: High CPU usage, input buffer overflowing
   - Fix: Call once per frame (not in tight loops)
   - Status in your code: ✓ Called once per frame

### Debugging Recommendations for Your Project

If input stops working:

1. **Verify focus** (GLFW window manager shows title bar activation)
2. **Check glfwPollEvents()** is still being called
3. **Verify window isn't hidden** (check with Alt+Tab)
4. **Test basic GLFW functions**:
   ```cpp
   printf("Focused: %d\n", glfwGetWindowAttrib(window, GLFW_FOCUSED));
   printf("W key: %d\n", glfwGetKey(window, GLFW_KEY_W));
   double x, y;
   glfwGetCursorPos(window, &x, &y);
   printf("Mouse: %.0f, %.0f\n", x, y);
   ```
5. **Add logging to callbacks** to verify they're being called
6. **Check that glfwMakeContextCurrent()** is still the active context

---

## Key Takeaways

### For Diagnosing Input Issues (Any Platform)

1. **Message pump is critical**
   - Must process messages every frame
   - Must call both TranslateMessage and DispatchMessage
   - Must not block on long operations

2. **Window visibility matters**
   - WS_VISIBLE must be set
   - Window must not be minimized
   - Window can receive input even while unfocused (platform-dependent)

3. **Focus affects keyboard**
   - Window must have focus for keyboard input
   - Mouse can work without focus (OS-dependent)
   - WM_KILLFOCUS should pause input

4. **Styles block input**
   - WS_EX_TRANSPARENT blocks ALL mouse input
   - WS_DISABLED blocks all input
   - WS_EX_NOACTIVATE prevents focus
   - WS_OVERLAPPEDWINDOW | WS_VISIBLE is safe

5. **Input can come from multiple sources**
   - Polling (GetAsyncKeyState / glfwGetKey)
   - Events (WM_KEYDOWN / callbacks)
   - Raw input (WM_INPUT / advanced)
   - Text input (WM_CHAR / special handling)

### For GLFW Projects (Like Yours)

1. **GLFW abstracts Win32 message pump**
   - You get "free" message processing
   - Still must call glfwPollEvents() every frame

2. **Two input patterns**
   - Polling: `glfwGetKey()` for continuous input
   - Events: Callbacks for discrete events
   - Mixing both is fine and recommended

3. **Focus loss is automatic**
   - GLFW handles WM_KILLFOCUS
   - Input stops automatically when window unfocused
   - Alt+Tab works correctly
   - Can be overridden with `glfwSetInputMode()`

4. **Mouse capture for games**
   - Toggle with `glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)`
   - Movement callback still fires
   - Your code implements this correctly with TAB toggle

---

## Quick Reference: Your Code

### Main Loop (Correct Pattern)
```cpp
// Location: src/main.cpp, line ~329
while (!glfwWindowShouldClose(window)) {
    // Calculate delta time
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - startTime;
    float currentFrame = elapsed.count();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // INPUT: Process input
    processInput(window);  // Polling
    glfwPollEvents();      // Event pump - CRITICAL

    // UPDATE and RENDER...

    // Swap buffers
    glfwSwapBuffers(window);
}
```

### Input Processing (Correct Pattern)
```cpp
// Location: src/main.cpp, line ~69
void processInput(GLFWwindow* window) {
    // Continuous input (polling)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);

    // Discrete input with state tracking
    static bool pausePressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pausePressed) {
        simulation->togglePause();
        pausePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        pausePressed = false;
    }

    // Mouse capture toggle
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        mouseCaptured = !mouseCaptured;
        if (mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        tabPressed = true;
    }
}
```

### Callbacks (Correct Pattern)
```cpp
// Location: src/main.cpp, line ~47
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mouseCaptured) return;
    // ... calculate delta and update camera
}

// Location: src/main.cpp, line ~186
glfwSetCursorPosCallback(window, mouse_callback);
glfwSetScrollCallback(window, scroll_callback);
```

---

## Recommended Reading Order

1. **Start here:** WIN32_INPUT_CHECKLIST.md
   - Get the quick diagnostic reference
   - Learn the critical things to check

2. **Then read:** WIN32_INPUT_DIAGNOSTICS.md
   - Understand the "why" behind each check
   - Learn about message pumps and styles

3. **Finally refer to:** WIN32_INPUT_CODE_EXAMPLES.md
   - When you need implementation examples
   - Copy patterns for your code

---

## Conclusion

Your OrganismEvolution project has a **correct input implementation**. The code follows best practices for GLFW-based games:

- ✓ Message pump running every frame (glfwPollEvents)
- ✓ Proper window creation and focus management
- ✓ Both polling and event-based input patterns
- ✓ Discrete action state tracking
- ✓ Mouse capture for camera control
- ✓ Clean separation of input and game logic

If you experience input issues, use the checklist in WIN32_INPUT_CHECKLIST.md to diagnose. The most likely causes (in order of probability):

1. Window lost focus (verify title bar shows focus indicator)
2. glfwPollEvents() temporarily removed (search for it in main loop)
3. Callback not registered (verify glfwSetXCallback calls are present)
4. Window minimized or hidden (Alt+Tab to check)
5. GLFW context changed (verify glfwMakeContextCurrent still active)

---

## Files Included in This Research Package

1. **WIN32_INPUT_DIAGNOSTICS.md** (~400 lines)
   - Comprehensive technical reference
   - All concepts explained in detail
   - Focus on understanding "why"

2. **WIN32_INPUT_CHECKLIST.md** (~300 lines)
   - Quick-reference checklist format
   - One-page summaries of key concepts
   - Fast diagnostic approach

3. **WIN32_INPUT_CODE_EXAMPLES.md** (~600 lines)
   - Practical, runnable code samples
   - Multiple implementation patterns
   - Solutions for common problems

4. **INPUT_RESEARCH_SUMMARY.md** (this file)
   - Overview of all resources
   - Assessment of your project
   - Quick reference to your code

---

**Created:** 2026-01-13
**Focus:** Win32 Window Input Handling Diagnostics
**Project:** OrganismEvolution (GLFW-based)
**Status:** Your implementation is correct

For questions or issues, refer to the specific document covering that topic.
