# Win32 Input Handling - Visual Reference Guide

## Message Flow Diagrams

### 1. Standard Win32 Message Pump Flow

```
User Action (Press W Key)
    ↓
Windows OS detects key press
    ↓
Posts WM_KEYDOWN to message queue
    ↓
GetMessage() retrieves from queue ← MESSAGE PUMP
    ↓
TranslateMessage() converts VK codes ← CRITICAL STEP
    ↓
DispatchMessage() routes to window ← CRITICAL STEP
    ↓
WindowProc() receives WM_KEYDOWN
    ↓
Application handles input
    ↓
Window procedure returns
    ↓
Back to GetMessage() for next message
```

### 2. GLFW Abstraction Layer

```
User Action (Press W Key)
    ↓
Windows OS detects key press
    ↓
Posts WM_KEYDOWN to GLFW's window
    ↓
GLFW Message Pump (Hidden)
    ├─ GetMessage()
    ├─ TranslateMessage()
    └─ DispatchMessage()
    ↓
GLFW processes the event
    ↓
glfwPollEvents() ← YOU MUST CALL THIS
    ↓
Application polls with glfwGetKey()
    ├─ OR Callback fires (glfwSetKeyCallback)
    ↓
Application handles input
    ↓
Back to glfwPollEvents()
```

### 3. Your OrganismEvolution Input Flow

```
User Presses WASD
    ↓
glfwPollEvents() ← Main loop, line 390
    ↓
GLFW's message pump processes it
    ↓
glfwGetKey() polls the state ← processInput(), line 75
    ↓
camera.processKeyboard() updates position
    ↓
Next frame
    ↓
Repeat
```

---

## Window Creation Check Flow

```
START: CreateWindow() call
    ↓
[Check] Style includes WS_OVERLAPPEDWINDOW?
    ├─ NO  → PROBLEM: Window may not be standard
    └─ YES → Continue
    ↓
[Check] Style includes WS_VISIBLE?
    ├─ NO  → PROBLEM: Window won't be visible
    └─ YES → Continue
    ↓
[Check] Extended style includes WS_EX_TRANSPARENT?
    ├─ YES → PROBLEM: No mouse input!
    └─ NO  → Continue
    ↓
[Check] Extended style includes WS_EX_NOACTIVATE?
    ├─ YES → PROBLEM: Can't get focus!
    └─ NO  → Continue
    ↓
[Check] Parent window parameter is NULL (for top-level)?
    ├─ NO  → Might be OK (child window), but check parent
    └─ YES → Continue
    ↓
[Check] ShowWindow(hwnd, SW_SHOW) called?
    ├─ NO  → PROBLEM: Window may not be visible
    └─ YES → Continue
    ↓
[Check] SetFocus(hwnd) called?
    ├─ NO  → Window might not have keyboard focus
    └─ YES → OK
    ↓
OK: Window should receive input
END
```

---

## Input Reception Troubleshooting Flow

```
PROBLEM: Input not working
    ↓
[Q1] Is the window visible and in focus?
    ├─ NO  → Click on window title bar to focus
    └─ YES → Continue
    ↓
[Q2] Is glfwPollEvents() called? (GLFW users)
    ├─ NO  → ADD THIS TO MAIN LOOP - CRITICAL!
    └─ YES → Continue
    ↓
[Q3] Is GetMessage/DispatchMessage working? (Native Win32)
    ├─ NO  → Check message pump implementation
    └─ YES → Continue
    ↓
[Q4] Add logging to window procedure
    ├─ Don't see logs? → Window procedure not being called
    │   └─ Check: Is DispatchMessage() being called?
    └─ See logs? → Continue
    ↓
[Q5] Are callbacks registered? (GLFW)
    ├─ NO  → Call glfwSetCursorPosCallback, glfwSetKeyCallback, etc.
    └─ YES → Continue
    ↓
[Q6] Is input being logged?
    ├─ NO  → Add printf statements to verify messages received
    └─ YES → Continue
    ↓
[Q7] Is application responding to input?
    ├─ NO  → Issue is in your application code, not input handling
    └─ YES → Input working!
    ↓
END
```

---

## Critical Code Locations Check

```
FOR: GLFW Projects
    ↓
[1] glfwInit()
    └─ Location: main(), before window creation
    └─ Status: ✓ (line 160 in your code)
    ↓
[2] glfwCreateWindow()
    └─ Location: main(), after init
    └─ Status: ✓ (line 171 in your code)
    ↓
[3] glfwMakeContextCurrent(window)
    └─ Location: main(), after window creation
    └─ Status: ✓ (line 185 in your code)
    ↓
[4] glfwSetXCallback() - All callbacks
    └─ Location: main(), after context
    └─ Status: ✓ (lines 186-188 in your code)
    ↓
[5] glfwPollEvents()
    └─ Location: MAIN LOOP ← CRITICAL
    └─ Status: ✓ (line 390 in your code)
    ↓
[6] glfwSwapBuffers()
    └─ Location: MAIN LOOP, after rendering
    └─ Status: ✓ (line 389 in your code)
    ↓
ALL CRITICAL ITEMS PRESENT ✓
```

---

## Window Styles Decision Tree

```
Creating a new window?
    ↓
[Q] What type of window?
    ├─ Regular application
    │   ├─ Style: WS_OVERLAPPEDWINDOW | WS_VISIBLE
    │   ├─ ExStyle: 0 (or WS_EX_APPWINDOW)
    │   └─ Parent: NULL
    │
    ├─ Game window (fullscreen)
    │   ├─ Style: WS_POPUP | WS_VISIBLE
    │   ├─ ExStyle: WS_EX_APPWINDOW
    │   └─ Parent: NULL
    │
    ├─ Transparent overlay
    │   ├─ Style: WS_POPUP | WS_VISIBLE
    │   ├─ ExStyle: WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST
    │   ├─ Parent: NULL
    │   └─ NOTE: Won't receive input due to WS_EX_TRANSPARENT
    │
    └─ Child window (in parent)
        ├─ Style: WS_CHILD | WS_VISIBLE
        ├─ ExStyle: 0
        ├─ Parent: Parent HWND
        └─ NOTE: Only gets focus if parent is active
```

---

## Message Processing State Machine

```
STATE: Idle (no messages)
    ↓
User presses key
    ↓
STATE: Message in Queue
    ↓
GetMessage() or PeekMessage() retrieves it
    ↓
STATE: Message Retrieved
    ↓
TranslateMessage() processes virtual key
    ↓
STATE: Message Translated
    ↓
DispatchMessage() routes to window procedure
    ↓
STATE: In Window Procedure
    ↓
Application handles the message
    ↓
Window procedure returns
    ↓
STATE: Message Processed
    ↓
Back to GetMessage() / PeekMessage()
    ↓
STATE: Idle (or next message)
```

---

## Input Method Comparison Chart

```
                    POLLING           EVENT-BASED      RAW INPUT
                 ┌─────────────┬──────────────────┬──────────────┐
Continuous Input│   GOOD      │   REQUIRES STATE │   EXCELLENT   │
                │ (GetAsync   │   (Track presses)│ (Low latency) │
                │  KeyState)  │                  │               │
                ├─────────────┼──────────────────┼──────────────┤
Discrete Input  │   POOR      │   EXCELLENT      │   OK          │
                │ (Can't tell │   (Event fires   │ (Requires     │
                │  release)   │    once)         │  interpretation)
                ├─────────────┼──────────────────┼──────────────┤
Text Input      │   NO        │   YES            │   NO          │
                │             │   (WM_CHAR)      │               │
                ├─────────────┼──────────────────┼──────────────┤
Works Unfocused │   YES       │   NO             │   YES*        │
                │ (Security   │ (Won't fire)     │ (*Registered  │
                │  risk!)     │                  │   correctly)  │
                ├─────────────┼──────────────────┼──────────────┤
CPU Usage       │   MEDIUM    │   LOW            │   MEDIUM      │
                │ (Polling    │   (Only on       │   (Processing)│
                │ every frame)│    events)       │               │
                ├─────────────┼──────────────────┼──────────────┤
Latency         │   VERY LOW  │   LOW            │   VERY LOW    │
                │ (Direct     │ (Queued)         │ (Direct)      │
                │  query)     │                  │               │
                └─────────────┴──────────────────┴──────────────┘

GLFW IMPLEMENTATION:
  Continuous (WASD): glfwGetKey()         ← Polling, GOOD
  Events (Click):    glfwSetKeyCallback() ← Event-based, EXCELLENT
  Mouse movement:    Callback             ← Event-based, EXCELLENT
```

---

## Focus State Diagram

```
[Window Created, Has Focus]
    ↓ User clicks another window
    ↓
[Window Loses Focus]
    │ WM_KILLFOCUS triggered
    │ Keyboard input STOPS
    │ Mouse input may continue (OS-dependent)
    ↓
[Waiting for Reactivation]
    │ User clicks back on window
    │ OR uses Alt+Tab to switch back
    ↓
[Window Gains Focus Again]
    │ WM_SETFOCUS triggered
    │ Keyboard input RESUMES
    ↓
[Window Has Focus, Input Working]
    ↓ User minimizes window
    ↓
[Window Minimized, Hidden]
    │ WM_KILLFOCUS might trigger
    │ All input effectively stops
    ↓
[Waiting for Window Restore]
    │ User clicks window in taskbar
    │ OR program calls ShowWindow(SW_SHOW)
    ↓
[Window Visible Again]
    │ Click it to regain focus
    ↓
[Back to Focus State]
```

---

## GetAsyncKeyState() Bit Structure

```
GetAsyncKeyState(VK_CODE)

Return Value: SHORT (16-bit)

┌──────────────────────────────────────┐
│ Bit 15 (Most Significant)            │ ← 1 = Key Currently DOWN
│ Bits 14-1 (Reserved)                 │
│ Bit 0 (Least Significant)            │ ← 1 = Key toggled since last call
└──────────────────────────────────────┘

Usage:
  if (GetAsyncKeyState(VK_W) & 0x8000) {
    // Key is currently down
  }

  if (GetAsyncKeyState(VK_CAPSLOCK) & 0x0001) {
    // Caps Lock was toggled since last call
  }
```

---

## WM_KEYDOWN lParam Structure

```
WM_KEYDOWN's lParam: 32-bit value

Bits 0-15:     Repeat Count
               ├─ 1 = Key first pressed
               └─ > 1 = Auto-repeat count

Bits 16-23:    Scan Code
               └─ Hardware scan code (rarely used)

Bit 24:        Extended Key Flag
               ├─ 0 = Standard key
               └─ 1 = Extended key (Right-Alt, Right-Ctrl, etc.)

Bits 25-28:    Reserved

Bit 29:        Context Code
               ├─ 0 = Menu key not pressed
               └─ 1 = Alt key was pressed

Bit 30:        Previous Key State
               ├─ 0 = Key previously up (First Press!)
               └─ 1 = Key previously down (Auto-repeat)

Bit 31:        Transition State
               ├─ 0 = Key down
               └─ 1 = Key up (for WM_KEYUP)

USAGE:
  bool firstPress = !(lParam & (1 << 30));
  if (firstPress) {
    // Only fires once per key press, not on auto-repeat
  }
```

---

## Virtual Key Code Reference

```
NAVIGATION:
  VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN

GAME CONTROLS (WASD):
  'W', 'A', 'S', 'D'  (uppercase letters)
  or VK_W, VK_A, VK_S, VK_D in some APIs

FUNCTION KEYS:
  VK_F1 through VK_F12

MODIFIERS:
  VK_SHIFT, VK_CONTROL, VK_MENU (Alt)
  VK_LSHIFT, VK_RSHIFT (Left/Right)
  VK_LCONTROL, VK_RCONTROL
  VK_LMENU, VK_RMENU (Left/Right Alt)

SPECIAL:
  VK_ESCAPE, VK_ENTER, VK_SPACE
  VK_TAB, VK_BACKSPACE, VK_DELETE
  VK_HOME, VK_END, VK_PAGEUP, VK_PAGEDOWN

MOUSE:
  VK_LBUTTON (Left Click)
  VK_RBUTTON (Right Click)
  VK_MBUTTON (Middle Click)
  VK_XBUTTON1, VK_XBUTTON2 (Extra buttons)
```

---

## Minimal Working Code Template

```cpp
// GLFW Version (Recommended)
int main() {
    glfwInit();                              // 1. Init
    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "Title", NULL, NULL       // 2. Create
    );
    glfwMakeContextCurrent(window);          // 3. Context

    // 4. Callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 5. Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();                    // ← CRITICAL

        // Polling input
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            // Handle W
        }

        // Render...
        glfwSwapBuffers(window);
    }

    glfwTerminate();                         // 6. Cleanup
    return 0;
}

// Win32 Version (Custom)
int main() {
    RegisterClass(...);                      // 1. Register
    HWND hwnd = CreateWindowEx(...);         // 2. Create
    SetFocus(hwnd);                          // 3. Focus

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);              // ← CRITICAL
        DispatchMessage(&msg);               // ← CRITICAL
    }

    return msg.wParam;
}
```

---

## Common Issues Quick Reference

```
SYMPTOM: No keyboard input
└─ CHECK: Is window focused? (Title bar should show focus)
└─ CHECK: Is GetMessage/DispatchMessage being called?
└─ CHECK: Is TranslateMessage being called?
└─ FIX: Call SetFocus(hwnd) after window creation

SYMPTOM: No mouse input
└─ CHECK: Is WS_EX_TRANSPARENT set? (FATAL - remove it)
└─ CHECK: Is WS_EX_NOACTIVATE set? (Dangerous - remove it)
└─ CHECK: Is the mouse over the window?
└─ FIX: Verify window styles don't have blocking flags

SYMPTOM: Input works then stops
└─ CHECK: Did window lose focus? (Alt+Tab, minimize, click elsewhere)
└─ CHECK: Is an infinite loop blocking the message pump?
└─ FIX: Ensure message pump runs every frame without blocking

SYMPTOM: Mouse movement slow/stuttering
└─ CHECK: Is processInput() doing heavy work?
└─ CHECK: Is message pump getting blocked?
└─ FIX: Move heavy work out of input handling

SYMPTOM: Text input not working
└─ CHECK: Is TranslateMessage being called?
└─ CHECK: Are you listening for WM_CHAR?
└─ FIX: Add case WM_CHAR to window procedure

SYMPTOM: Callbacks never fire (GLFW)
└─ CHECK: Is glfwPollEvents() being called?
└─ CHECK: Are callbacks registered with glfwSetXCallback?
└─ FIX: Add both to main loop
```

---

## Summary: The Three Rules of Win32 Input

```
┌─────────────────────────────────────────────────────────┐
│ RULE 1: MESSAGE PUMP                                    │
│ ─────────────────────────────────────────────────────── │
│ Every frame, call:                                       │
│   GetMessage() or PeekMessage()  ← Retrieve             │
│   TranslateMessage()             ← Convert              │
│   DispatchMessage()              ← Route                │
│                                                          │
│ Without this, no input works!                            │
│ (GLFW does this for you, but you must call glfwPollEvents) │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ RULE 2: CORRECT STYLES                                  │
│ ─────────────────────────────────────────────────────── │
│ Required:                                                │
│   WS_OVERLAPPEDWINDOW | WS_VISIBLE                       │
│                                                          │
│ Forbidden:                                               │
│   WS_EX_TRANSPARENT (blocks mouse)                       │
│   WS_EX_NOACTIVATE (blocks focus)                        │
│   WS_DISABLED (blocks all input)                         │
│                                                          │
│ Without this, input doesn't work!                        │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ RULE 3: WINDOW FOCUS                                    │
│ ─────────────────────────────────────────────────────── │
│ Window must have focus for keyboard input.              │
│ Call SetFocus(hwnd) after creation.                     │
│ Handle WM_KILLFOCUS and WM_SETFOCUS properly.          │
│                                                          │
│ Without this, keyboard input stops when unfocused!       │
└─────────────────────────────────────────────────────────┘
```

---

**All diagrams are based on Win32 internals and GLFW abstraction**
**Current as of 2026-01-13**
