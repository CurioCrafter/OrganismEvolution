# Win32 Window Input Handling Diagnostics Guide

## Overview
This document provides comprehensive information about Win32 window input handling, common issues, and diagnostic techniques. While your project uses GLFW (which abstracts Win32), understanding the underlying Win32 mechanisms is crucial for diagnosing input problems.

---

## Part 1: Common Reasons a Win32 Window Doesn't Receive Input

### 1.1 Missing or Broken Message Pump
**The Core Problem:** Win32 applications must have a functional message pump to receive input events.

#### Correct Message Pump Pattern:
```cpp
MSG msg = {0};
while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);  // Convert virtual key codes to character messages
    DispatchMessage(&msg);   // Send message to window procedure
}
```

#### Common Issues:
- **Missing TranslateMessage()**: Without this, WM_CHAR messages won't be generated, breaking text input
- **Missing DispatchMessage()**: Without this, messages never reach your window procedure
- **Improper loop termination**: GetMessage() returns 0 when WM_QUIT is received
- **Blocking operations in message pump**: Long-running operations block input processing
- **Not polling in game loops**: Game engines typically use PeekMessage() for non-blocking checks:

```cpp
MSG msg = {0};
while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
// Continue game logic
```

### 1.2 Window Style Issues

#### Styles That Block Input:
| Style Flag | Effect | Solution |
|------------|--------|----------|
| WS_DISABLED | Window cannot receive input | Use WS_OVERLAPPEDWINDOW instead |
| WS_EX_TRANSPARENT | Window ignores all mouse input | Remove this extended style |
| WS_EX_LAYERED | May interfere with input if not properly configured | Use caution with this |
| Missing WS_VISIBLE | Window exists but isn't drawn/active | Ensure WS_VISIBLE is set |
| Missing WS_CHILD parent | Child window won't receive focus | Ensure parent window is created first |

#### Required Styles for Input:
```cpp
// Typical window creation with proper styles for input
HWND hwnd = CreateWindowEx(
    0,                              // Extended style (usually 0)
    CLASS_NAME,
    WINDOW_TITLE,
    WS_OVERLAPPEDWINDOW |           // WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                                    // WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
    WS_VISIBLE,                     // Must include this for window to be visible and active
    CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL
);
```

#### Extended Styles to Avoid:
```cpp
// AVOID these combinations:
WS_EX_TRANSPARENT      // Makes window ignore input
WS_EX_NOACTIVATE       // Window won't get focus
WS_EX_LAYERED          // Requires special handling

// GOOD extended styles for input:
WS_EX_APPWINDOW        // Window shows in taskbar
WS_EX_WINDOWEDGE       // Adds 3D border
```

### 1.3 Focus Issues

#### Focus Requirement:
- A window must have **keyboard focus** to receive keyboard input
- A window must be **under the cursor** to receive mouse input
- Parent window must be active for child windows to receive focus

#### Checking Focus:
```cpp
// Get currently focused window
HWND focusedWindow = GetFocus();
if (focusedWindow != hwnd) {
    // This window doesn't have focus - won't receive keyboard input
    MessageBox(NULL, L"Window lost focus", L"Alert", MB_OK);
    SetFocus(hwnd);  // Try to regain focus
}
```

#### Ensuring Focus:
```cpp
// In your window procedure
case WM_CREATE:
    SetFocus(hwnd);  // Give focus when created
    break;

case WM_ACTIVATE:
    if (LOWORD(wParam) != WA_INACTIVE) {
        SetFocus(hwnd);  // Regain focus if activated
    }
    break;

case WM_KILLFOCUS:
    // Pause game or reset input state
    inputActive = false;
    break;
```

### 1.4 Message Pump Problems

#### Issue: Application Freezes
**Cause:** Long operation in message handler blocks the message pump
```cpp
// BAD: Blocks message pump
case WM_PAINT:
    Sleep(5000);  // Window becomes unresponsive
    break;

// GOOD: Offload to separate thread
case WM_PAINT:
    PostMessage(hwnd, WM_APP + 1, 0, 0);  // Post custom message
    break;
```

#### Issue: Input Lag
**Cause:** Message queue fills up because DispatchMessage takes too long
```cpp
// Better approach for game loops:
while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
// Game update happens here without blocking input
```

#### Issue: Lost Input Events
**Cause:** Input events are being dropped because the queue is full or events arrive faster than they're processed
```cpp
// Solution: Dequeue input immediately
case WM_KEYDOWN:
    // Process immediately, don't do heavy lifting
    SetEvent(inputEvent);
    break;
```

---

## Part 2: Required Window Styles for Input

### 2.1 Window Style Checklist

```cpp
// Minimal required styles for input-enabled window:
DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
DWORD dwExStyle = 0;  // Start with no extended styles

// Add extended styles if needed:
// dwExStyle |= WS_EX_APPWINDOW;    // Shows in taskbar
// dwExStyle |= WS_EX_WINDOWEDGE;   // 3D border
```

### 2.2 Extended Styles Explained

```cpp
// Input-friendly extended styles:
WS_EX_APPWINDOW      // Task bar shows this window (good for main windows)
WS_EX_WINDOWEDGE     // 3D border (purely aesthetic, doesn't affect input)
WS_EX_CLIENTEDGE     // 3D border on client area

// Input-hostile extended styles:
WS_EX_TRANSPARENT    // BLOCKS ALL MOUSE INPUT - never use for active windows
WS_EX_NOACTIVATE     // Window can't receive focus
WS_EX_TOOLWINDOW     // Doesn't activate when clicked
WS_EX_TOPMOST        // Always on top (use sparingly)
WS_EX_LAYERED        // For transparency/alpha blending - complex input handling
```

### 2.3 Focus-Related Styles

```cpp
// Ensure focus can be acquired:
HWND hwnd = CreateWindowEx(
    0,                           // No problematic extended styles
    CLASS_NAME,
    WINDOW_TITLE,
    WS_OVERLAPPEDWINDOW |        // Base styles
    WS_VISIBLE,                  // CRITICAL: Must be visible
    CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT,
    NULL,                        // No parent window
    NULL,                        // No menu
    hInstance,
    NULL
);

if (hwnd) {
    SetFocus(hwnd);             // Give initial focus
    ShowWindow(hwnd, SW_SHOW);  // Ensure it's shown
}
```

---

## Part 3: GetAsyncKeyState vs WM_KEYDOWN Messages

### 3.1 GetAsyncKeyState() - Polling Method

#### How It Works:
```cpp
// Polls the keyboard state synchronously (not message-based)
if (GetAsyncKeyState(VK_W) & 0x8000) {
    // W key is currently pressed
    camera.moveForward();
}
```

#### Characteristics:
| Aspect | Details |
|--------|---------|
| **When to use** | Continuous movement (camera controls, smooth motion) |
| **Return value** | -32768 (0x8000) if pressed, bit 0 set if toggled since last call |
| **Blocking** | Non-blocking, directly queries OS keyboard state |
| **Latency** | Direct query (very low latency) |
| **Precision** | Only checks if currently pressed, not duration |
| **Works outside window** | YES - even if your window is unfocused |
| **Platform** | Windows only |

#### Advantages:
- Very fast for continuous key state
- Works even when window is unfocused (be careful!)
- Low latency
- Perfect for real-time input (WASD movement)

#### Disadvantages:
- Doesn't help with text input
- Can't distinguish between press and release events
- Works outside window (security concern)
- Not message-based

#### Example:
```cpp
// From your main.cpp - this is the correct pattern!
void processInput(GLFWwindow* window) {
    // These use glfwGetKey() which is similar to GetAsyncKeyState
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    // ... etc
}
```

### 3.2 WM_KEYDOWN Messages - Event-Based Method

#### How It Works:
```cpp
// In your window procedure
case WM_KEYDOWN:
    {
        int nVirtKey = (int)wParam;      // Virtual key code
        int lKeyData = (long)lParam;     // Key data

        // Extract information from lParam
        int scanCode = (lKeyData >> 16) & 0xff;
        int repeatCount = lKeyData & 0xffff;
        int isExtended = (lKeyData >> 24) & 0x01;

        printf("Key: %d, Repeat: %d, Extended: %d\n",
               nVirtKey, repeatCount, isExtended);
        break;
    }
```

#### Characteristics:
| Aspect | Details |
|--------|---------|
| **When to use** | One-time actions (fire button, pause toggle) |
| **Message frequency** | One per key press, then repeats if held |
| **Auto-repeat** | YES - key repeats after OS repeat delay (~500ms) then every ~33ms |
| **Latency** | Queued through message loop (slight delay) |
| **Requires focus** | YES - only works when window has focus |
| **Works outside window** | NO - window must be focused |
| **Platform** | Windows only (GLFW abstracts this) |

#### WM_KEYDOWN lParam Structure:
```
Bits 0-15:   Repeat count
Bits 16-23:  Scan code
Bit 24:      Extended key flag (1 = extended)
Bits 25-28:  Reserved
Bit 29:      Context code (1 = Alt was pressed)
Bit 30:      Previous key state (1 = key was down)
Bit 31:      Transition state (0 = pressed, 1 = released)
```

#### Advantages:
- Event-driven (doesn't waste cycles polling)
- Has key repeat information
- Accurate press/release timing
- Message-queued (proper ordering)
- Works with text input (WM_CHAR)

#### Disadvantages:
- Requires message loop
- Auto-repeat can complicate logic
- Only works when focused
- Need to track key state manually for continuous actions

#### Example Usage Pattern:
```cpp
// For discrete actions (good pattern)
case WM_KEYDOWN:
    if (wParam == VK_SPACE) {
        if (!(lParam & (1 << 30))) {  // Check if this isn't a repeat
            fireWeapon();
        }
    }
    break;

// For continuous movement (bad pattern - use polling instead)
case WM_KEYDOWN:
    if (wParam == 'W') {
        isMovingForward = true;
    }
    break;

case WM_KEYUP:
    if (wParam == 'W') {
        isMovingForward = false;
    }
    break;
```

### 3.3 Comparison Table

| Feature | GetAsyncKeyState | WM_KEYDOWN |
|---------|------------------|-----------|
| **Input type** | Polling | Event-driven |
| **Continuous input** | Excellent | Poor (need tracking) |
| **Discrete input** | Poor (hard to detect press) | Excellent |
| **Text input** | Can't use | Perfect (with WM_CHAR) |
| **Low latency** | Yes | Slight delay |
| **Works unfocused** | Yes (dangerous) | No |
| **Key repeat** | Not applicable | Automatic |
| **Scan code available** | No | Yes |
| **Extended key info** | No | Yes |

### 3.4 GLFW Abstraction (Your Current Approach)

Your project uses GLFW, which provides high-level functions that abstract both approaches:

```cpp
// From your main.cpp
void processInput(GLFWwindow* window) {
    // This is similar to GetAsyncKeyState - polling continuous state
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
}

// GLFW callbacks for event-based input
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // This is event-driven (like WM_KEYDOWN)
    camera.processMouseScroll(yoffset);
}
```

---

## Part 4: Debug Techniques for Win32 Input Issues

### 4.1 Basic Debugging Tools

#### Check Window Focus:
```cpp
HWND focusedWindow = GetFocus();
bool isFocused = (focusedWindow == hwnd);
printf("Window focused: %s\n", isFocused ? "Yes" : "No");
```

#### Check Window Visibility:
```cpp
bool isVisible = IsWindowVisible(hwnd);
printf("Window visible: %s\n", isVisible ? "Yes" : "No");
```

#### Check Window Style:
```cpp
LONG style = GetWindowLongA(hwnd, GWL_STYLE);
LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

printf("Style: 0x%08X\n", style);
printf("Extended Style: 0x%08X\n", exStyle);

// Check specific styles
if (style & WS_VISIBLE) printf("  - WS_VISIBLE\n");
if (style & WS_DISABLED) printf("  - WS_DISABLED (PROBLEM!)\n");
if (exStyle & WS_EX_TRANSPARENT) printf("  - WS_EX_TRANSPARENT (PROBLEM!)\n");
```

#### Check Message Queue:
```cpp
// Count messages in queue
QUEUEINFO queueInfo;
GetQueueStatus(QS_ALLINPUT);  // Returns count of pending messages
```

### 4.2 Logging Input Events

#### Simple Message Pump Logging:
```cpp
MSG msg = {0};
while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    // Log all messages
    switch (msg.message) {
        case WM_KEYDOWN:
            printf("[INPUT] Key Down: %d\n", msg.wParam);
            break;
        case WM_KEYUP:
            printf("[INPUT] Key Up: %d\n", msg.wParam);
            break;
        case WM_MOUSEMOVE:
            printf("[INPUT] Mouse Move: %d, %d\n", GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
            break;
        case WM_LBUTTONDOWN:
            printf("[INPUT] Left Click Down\n");
            break;
        case WM_LBUTTONUP:
            printf("[INPUT] Left Click Up\n");
            break;
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
```

#### Window Procedure Logging:
```cpp
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Only log input messages to reduce spam
    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) {
        printf("[WM_] %s (0x%04X)\n", GetMessageName(uMsg), uMsg);
    }
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
        printf("[WM_] %s (0x%04X)\n", GetMessageName(uMsg), uMsg);
    }

    // ... rest of handler
}

// Helper to get human-readable message names
const char* GetMessageName(UINT msg) {
    switch (msg) {
        case WM_KEYDOWN: return "WM_KEYDOWN";
        case WM_KEYUP: return "WM_KEYUP";
        case WM_CHAR: return "WM_CHAR";
        case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
        case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
        // ... etc
        default: return "UNKNOWN";
    }
}
```

### 4.3 Timing Issues

#### Check for Message Pump Blocking:
```cpp
auto messageStartTime = std::chrono::high_resolution_clock::now();

MSG msg = {0};
while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

auto messageEndTime = std::chrono::high_resolution_clock::now();
auto messageDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
    messageEndTime - messageStartTime
);

if (messageDuration.count() > 16) {
    printf("WARNING: Message pump took %lld ms (frame time budget exceeded)\n",
           messageDuration.count());
}
```

### 4.4 Focus and Activation Issues

#### Detect Focus Loss:
```cpp
case WM_KILLFOCUS:
    printf("[FOCUS] Lost focus - pausing input\n");
    inputEnabled = false;
    return 0;

case WM_SETFOCUS:
    printf("[FOCUS] Gained focus - resuming input\n");
    inputEnabled = true;
    return 0;

case WM_ACTIVATE:
    if (LOWORD(wParam) != WA_INACTIVE) {
        printf("[FOCUS] Window activated\n");
    } else {
        printf("[FOCUS] Window deactivated\n");
    }
    return 0;
```

#### Monitor Cursor Position:
```cpp
POINT cursorPos;
GetCursorPos(&cursorPos);
printf("Cursor position: %d, %d\n", cursorPos.x, cursorPos.y);

// Convert to client coordinates
ScreenToClient(hwnd, &cursorPos);
printf("Client coordinates: %d, %d\n", cursorPos.x, cursorPos.y);
```

---

## Checklist: Verifying Win32 Window Input

Use this comprehensive checklist when debugging input issues:

### Phase 1: Window Creation and Visibility
- [ ] Window is created successfully (hwnd is not NULL)
- [ ] Window uses WS_OVERLAPPEDWINDOW or similar (not missing WS_OVERLAPPED)
- [ ] Window includes WS_VISIBLE flag
- [ ] Extended styles don't include WS_EX_TRANSPARENT
- [ ] Extended styles don't include WS_EX_NOACTIVATE
- [ ] Window is shown with ShowWindow(hwnd, SW_SHOW)
- [ ] Window is visible on screen (visually confirmed)

### Phase 2: Window Focus and Activation
- [ ] Initial focus is set with SetFocus(hwnd) after creation
- [ ] Window can be clicked to gain focus (not WS_EX_NOACTIVATE)
- [ ] WM_KILLFOCUS is handled correctly (not disabling window)
- [ ] WM_SETFOCUS is handled (optional: can reset input state)
- [ ] WM_ACTIVATE handles both WA_INACTIVE and activation cases
- [ ] GetFocus() returns hwnd when testing
- [ ] Window title bar is fully visible and clickable

### Phase 3: Message Pump
- [ ] GetMessage() or PeekMessage() is called in main loop
- [ ] TranslateMessage() is called for every message
- [ ] DispatchMessage() is called for every message
- [ ] Message pump doesn't block (no Sleep or long operations)
- [ ] Message pump processes messages from all sources
- [ ] Return value of GetMessage() is checked properly
- [ ] Loop properly terminates on WM_QUIT

### Phase 4: Keyboard Input
- [ ] WM_KEYDOWN is received (check by logging)
- [ ] WM_KEYUP is received (check by logging)
- [ ] Key codes are interpreted correctly (VK_* constants)
- [ ] Auto-repeat isn't interfering (check lParam bit 30)
- [ ] Alt key combinations work (WM_SYSKEYDOWN for Alt+key)
- [ ] Text input uses WM_CHAR (not just WM_KEYDOWN)
- [ ] Non-ASCII characters are translated (TranslateMessage required)

### Phase 4a: If Using GetAsyncKeyState (Polling)
- [ ] GetAsyncKeyState is called with correct VK_* codes
- [ ] Return value is properly masked (& 0x8000)
- [ ] Key state is checked every frame (continuous polling)
- [ ] Function is called from correct thread (main thread only)

### Phase 5: Mouse Input
- [ ] WM_MOUSEMOVE is received
- [ ] WM_LBUTTONDOWN is received
- [ ] WM_LBUTTONUP is received
- [ ] WM_RBUTTONDOWN and WM_RBUTTONUP work
- [ ] WM_MBUTTONDOWN and WM_MBUTTONUP work (if supported)
- [ ] WM_MOUSEWHEEL is received (and has correct scroll direction)
- [ ] Mouse coordinates are correctly extracted from lParam
- [ ] GET_X_LPARAM and GET_Y_LPARAM macros are used correctly

### Phase 6: Raw Input (Advanced - if standard input fails)
- [ ] RegisterRawInputDevices() is called with correct devices
- [ ] WM_INPUT is received and parsed
- [ ] Raw input data is correctly extracted
- [ ] Relative mouse motion is calculated if needed

### Phase 7: Performance and Responsiveness
- [ ] Message pump completes in < 1ms per frame (typical)
- [ ] No message queue overflow (queue capacity is limited)
- [ ] Input latency is < 16ms at 60 FPS
- [ ] Frame rate doesn't drop due to input processing
- [ ] No "application not responding" errors

### Phase 8: Special Cases
- [ ] Alt+Tab switching works (window loses and regains focus)
- [ ] Alt+F4 closes window properly
- [ ] Window resizing doesn't block input
- [ ] Full-screen toggle handles input correctly
- [ ] Input works after screen sleep/wake
- [ ] Input works on all monitors in multi-monitor setup

---

## Part 5: GLFW-Specific Notes (Your Current Setup)

Your project uses GLFW, which abstracts away Win32 details. However, understanding the underlying mechanism is useful:

### 5.1 GLFW Input Model

```cpp
// From your main.cpp:

// Method 1: Polling (like GetAsyncKeyState)
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Called every frame - this is polling
        camera.processKeyboard(FORWARD, deltaTime);
    }
}

// Method 2: Event callbacks (like WM_* messages)
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Called when mouse moves - event-driven
    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Called when scrolling - event-driven
    camera.processMouseScroll(yoffset);
}
```

### 5.2 Potential GLFW Issues

If GLFW isn't receiving input:

#### 1. Window Context Not Set:
```cpp
// CRITICAL: Must be done before using GLFW input functions
glfwMakeContextCurrent(window);  // Your code has this - good!
```

#### 2. Callbacks Not Registered:
```cpp
// Your code does this correctly:
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
glfwSetCursorPosCallback(window, mouse_callback);
glfwSetScrollCallback(window, scroll_callback);

// If mouse callback never fires, this is the reason!
```

#### 3. Input Mode Issues:
```cpp
// Your code handles this:
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // For captured mouse
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);    // For normal mouse
```

#### 4. Poll Events Not Called:
```cpp
// YOUR CODE MUST HAVE THIS in the main loop:
glfwPollEvents();  // Your code has this - good!
// Without this, callbacks are never invoked
```

### 5.3 Common GLFW Input Problems

| Problem | Cause | Solution |
|---------|-------|----------|
| Callbacks never fire | glfwPollEvents() not called | Add glfwPollEvents() in main loop |
| glfwGetKey returns GLFW_RELEASE always | Poll after glfwPollEvents() | Move poll into main loop |
| Mouse callback doesn't fire | Callback not registered | Call glfwSetCursorPosCallback |
| Mouse input seems delayed | Input processed too slowly | Reduce work in callbacks |
| Input stops working | Window lost focus/context | Check glfwGetCurrentContext |

---

## Quick Reference: Win32 Virtual Key Codes

### Common Keys Used in Games:
```cpp
VK_ESCAPE       // ESC key
VK_SPACE        // Spacebar
VK_RETURN       // Enter key
VK_TAB          // Tab key

// Letters (A-Z)
VK_A through VK_Z

// Numbers (0-9)
VK_0 through VK_9

// Function keys
VK_F1 through VK_F12

// Arrow keys
VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN

// Modifiers
VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU

// Mouse buttons
VK_LBUTTON      // Left mouse button
VK_RBUTTON      // Right mouse button
VK_MBUTTON      // Middle mouse button
```

---

## Summary

### When a Win32 Window Doesn't Receive Input:

1. **Check the message pump first** - GetMessage/PeekMessage, TranslateMessage, DispatchMessage
2. **Verify window styles** - WS_VISIBLE must be present, WS_DISABLED/WS_EX_TRANSPARENT must not be present
3. **Confirm focus** - Use GetFocus() to verify the window has keyboard focus
4. **Check callback registration** - In GLFW, ensure all callbacks are registered
5. **Verify glfwPollEvents()** - This must be called every frame in GLFW applications
6. **Monitor for blocking operations** - Don't do heavy work in message handlers
7. **Test focus management** - Handle WM_KILLFOCUS and WM_SETFOCUS correctly

### Key Differences for Your Use Case:

Since you're using GLFW, you're protected from most Win32 complexity. However:
- Your processInput() function is a polling approach (like GetAsyncKeyState)
- Your callbacks are event-driven (like WM_* messages)
- GLFW's glfwPollEvents() handles the Win32 message pump internally
- The critical line is `glfwPollEvents()` - without it, no input callbacks fire

Your current implementation looks correct. If input stops working, check:
1. Is glfwPollEvents() still being called?
2. Is the window still focused?
3. Are callbacks still registered?
4. Has glfwMakeContextCurrent() been called?
