# Win32 Window Input - Quick Diagnostic Checklist

## One-Page Reference for Fixing Input Problems

### CRITICAL ISSUES (Check These First)

- [ ] **Message pump exists and runs every frame**
  - `while (GetMessage(&msg, ...) > 0)` or `PeekMessage(&msg, ..., PM_REMOVE)`
  - `TranslateMessage(&msg);`
  - `DispatchMessage(&msg);`

- [ ] **glfwPollEvents() called in main loop** (for GLFW users)
  ```cpp
  while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();  // MUST be here!
      // ... rendering code
  }
  ```

- [ ] **Window has focus**
  - Test: `GetFocus() == hwnd`
  - Ensure: `SetFocus(hwnd)` called after window creation
  - Handle: `WM_KILLFOCUS` and `WM_SETFOCUS`

- [ ] **Window is visible**
  - Test: `IsWindowVisible(hwnd) == true`
  - Ensure: `WS_VISIBLE` flag set in CreateWindow
  - Ensure: `ShowWindow(hwnd, SW_SHOW)` called

---

## Window Creation (Win32 Direct)

- [ ] Correct window style:
  ```cpp
  DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;  // GOOD
  DWORD dwStyle = WS_POPUP;  // RISKY - no title bar
  ```

- [ ] NO problematic extended styles:
  - [ ] NOT `WS_EX_TRANSPARENT` (blocks mouse)
  - [ ] NOT `WS_EX_NOACTIVATE` (blocks focus)
  - [ ] NOT `WS_EX_LAYERED` without special handling

- [ ] CreateWindowEx parameters:
  ```cpp
  CreateWindowEx(
      0,                              // Extended style (usually 0)
      CLASS_NAME,
      WINDOW_TITLE,
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Style
      CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT,
      NULL,                           // Parent (must be NULL for top-level)
      NULL,                           // Menu
      hInstance,
      NULL
  );
  ```

---

## Message Pump Verification

### For Blocking Message Loop:
```cpp
MSG msg = {0};
while (GetMessage(&msg, NULL, 0, 0) > 0) {
    printf("[MSG] %04X\n", msg.message);  // Add this for debugging

    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
```

Checklist:
- [ ] Check return value of GetMessage (0 = WM_QUIT)
- [ ] TranslateMessage called before DispatchMessage
- [ ] Messages are being printed to console (add debugging)
- [ ] No long operations block the loop

### For Non-Blocking Message Loop (Game Pattern):
```cpp
MSG msg = {0};
while (!shouldExit) {  // Game loop
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            shouldExit = true;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Game update/render happens here
    UpdateGame();
    RenderFrame();
}
```

Checklist:
- [ ] PeekMessage returns immediately (PM_REMOVE flag set)
- [ ] Messages are logged (verify they're being received)
- [ ] Game loop continues even if no messages
- [ ] No blocking calls in game update

---

## GLFW-Specific Checklist

- [ ] Window created: `glfwCreateWindow(width, height, title, NULL, NULL)`
- [ ] Context set: `glfwMakeContextCurrent(window)`
- [ ] Callbacks registered:
  - [ ] `glfwSetKeyCallback(window, key_callback)` - if using callbacks
  - [ ] `glfwSetCursorPosCallback(window, mouse_callback)` - for mouse movement
  - [ ] `glfwSetScrollCallback(window, scroll_callback)` - for scroll wheel
  - [ ] `glfwSetMouseButtonCallback(window, mouse_button_callback)` - for clicks
  - [ ] `glfwSetFramebufferSizeCallback(window, framebuffer_callback)` - for resize

- [ ] **CRITICAL**: `glfwPollEvents()` in main loop:
  ```cpp
  while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();  // Without this, NO input!
      // ... rest of code
  }
  ```

- [ ] **CRITICAL**: `glfwSwapBuffers(window)` after rendering:
  ```cpp
  // Render...
  glfwSwapBuffers(window);  // Needed for some input handling
  glfwPollEvents();  // Then poll
  ```

- [ ] Polling for key state: `glfwGetKey(window, key) == GLFW_PRESS`
- [ ] Don't mix immediate and callback-based input poorly
- [ ] Window should be focused before expecting input (click on it)

---

## Input Reception Verification

### Keyboard Input:
- [ ] WM_KEYDOWN is received (add logging to window proc):
  ```cpp
  case WM_KEYDOWN:
      printf("KEY DOWN: %d\n", wParam);  // Add this
      break;
  ```

- [ ] Console shows key codes when you press keys
- [ ] glfwGetKey returns GLFW_PRESS when key held
- [ ] Different keys produce different codes

### Mouse Input:
- [ ] WM_MOUSEMOVE is received
  ```cpp
  case WM_MOUSEMOVE:
      printf("MOUSE: %d, %d\n", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      break;
  ```

- [ ] WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN received
- [ ] glfwSetCursorPosCallback fires when mouse moves
- [ ] glfwSetMouseButtonCallback fires on clicks

### Scroll Wheel:
- [ ] WM_MOUSEWHEEL is received
- [ ] glfwSetScrollCallback fires when scrolling
- [ ] Positive/negative values indicate direction

---

## Focus Management

In window procedure:
```cpp
case WM_KILLFOCUS:
    // Window lost focus - INPUT STOPS HERE
    printf("LOST FOCUS\n");
    inputEnabled = false;
    break;

case WM_SETFOCUS:
    // Window gained focus - INPUT RESUMES HERE
    printf("GAINED FOCUS\n");
    inputEnabled = true;
    break;

case WM_ACTIVATE:
    // WA_INACTIVE = minimized or another window active
    // WA_ACTIVE or WA_CLICKACTIVE = user activated this window
    if (LOWORD(wParam) != WA_INACTIVE) {
        printf("WINDOW ACTIVE\n");
        SetFocus(hwnd);  // Ensure we have focus
    }
    break;
```

---

## Debug: What to Log

### Minimal Logging (in window procedure):
```cpp
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Only log if there's a problem - disable in production
    #ifdef DEBUG_INPUT
    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) {
        printf("[WM] Key: %04X, Type: %s\n", wParam,
               uMsg == WM_KEYDOWN ? "DOWN" : uMsg == WM_KEYUP ? "UP" : "CHAR");
    }
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
        printf("[WM] Mouse: %04X\n", uMsg);
    }
    if (uMsg == WM_KILLFOCUS || uMsg == WM_SETFOCUS || uMsg == WM_ACTIVATE) {
        printf("[WM] Focus: %04X\n", uMsg);
    }
    #endif

    // ... rest of handler
}
```

### Minimal GLFW Logging:
```cpp
void debug_input() {
    // Call this once per frame to check input state
    static int lastW = GLFW_RELEASE;
    int currW = glfwGetKey(window, GLFW_KEY_W);

    if (currW != lastW) {
        printf("[INPUT] W key: %s\n", currW == GLFW_PRESS ? "PRESSED" : "RELEASED");
        lastW = currW;
    }
}
```

---

## Common Problems and Solutions

| Problem | Likely Cause | Fix |
|---------|-------------|-----|
| No keyboard input | Window lost focus | Click on window, check WM_KILLFOCUS handling |
| No mouse input | WS_EX_TRANSPARENT set | Remove that extended style |
| Callbacks never fire | glfwPollEvents not called | Add it to main loop |
| Input freezes | Blocking operation in message handler | Move heavy work to another thread |
| Input works then stops | Message queue overflow | Check frame rate, optimize message handling |
| Keys repeat too fast | WM_KEYDOWN auto-repeat | Check bit 30 of lParam in WM_KEYDOWN |
| Mouse moves slowly | Running in background | Use raw input (WM_INPUT) instead |
| Inconsistent input latency | Message pump not optimized | Minimize work in message handlers |
| Input works at startup, fails later | Lost focus after alt-tab | Handle WM_SETFOCUS properly |

---

## The Essential Win32 Input Code Pattern

```cpp
// 1. Window creation
HWND hwnd = CreateWindowEx(
    0,
    CLASS_NAME,
    "Window Title",
    WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // KEY: WS_VISIBLE required!
    CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL
);

if (!hwnd) {
    MessageBoxA(NULL, "CreateWindow failed", "Error", MB_OK);
    return 1;
}

// 2. Set initial focus
SetFocus(hwnd);
ShowWindow(hwnd, SW_SHOW);

// 3. Message pump
MSG msg = {0};
while (GetMessage(&msg, NULL, 0, 0) > 0) {  // Blocks until message
    TranslateMessage(&msg);    // Convert VK_* to WM_CHAR
    DispatchMessage(&msg);     // Send to window procedure
}

// Or for games (non-blocking):
while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {  // Returns immediately
    if (msg.message == WM_QUIT) break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

// 4. Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            HandleKeyDown(wParam, lParam);
            return 0;

        case WM_KEYUP:
            HandleKeyUp(wParam, lParam);
            return 0;

        case WM_CHAR:
            HandleChar(wParam, lParam);  // Text input
            return 0;

        case WM_LBUTTONDOWN:
            HandleMouseClick();
            return 0;

        case WM_MOUSEMOVE:
            HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_KILLFOCUS:
            HandleFocusLoss();
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
```

---

## The Essential GLFW Input Code Pattern

```cpp
// 1. Initialize GLFW
glfwInit();

// 2. Create window
GLFWwindow* window = glfwCreateWindow(1280, 720, "Title", NULL, NULL);

// 3. Set context current
glfwMakeContextCurrent(window);

// 4. Register callbacks (optional but good)
glfwSetCursorPosCallback(window, mouse_callback);
glfwSetScrollCallback(window, scroll_callback);

// 5. Main loop
while (!glfwWindowShouldClose(window)) {
    // CRITICAL: Call this EVERY frame
    glfwPollEvents();

    // Polling input (continuous keys)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // W is pressed
    }

    // Render...

    glfwSwapBuffers(window);
}

// Callbacks are event-driven
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Called when mouse moves (no polling needed)
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Called when scrolling
}
```

---

## Final Checklist Before Declaring "Input Works"

- [ ] **Window is focused** (title bar shows input focus indicator)
- [ ] **WASD keys** move camera smoothly
- [ ] **Mouse movement** when captured works
- [ ] **Left click** registers
- [ ] **Scroll wheel** zooms
- [ ] **ESC** closes window
- [ ] **Alt+Tab** switches out and back (focus regained)
- [ ] **Window resize** doesn't freeze input
- [ ] **Input works after 10+ minutes** of continuous use
- [ ] **No "not responding"** errors appear

If all these pass, input is working correctly.
