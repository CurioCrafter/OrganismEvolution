# Win32 Input Handling - Practical Code Examples

## Table of Contents
1. Basic Message Pump Examples
2. Window Procedure Input Handlers
3. Input State Tracking
4. GetAsyncKeyState Examples
5. Raw Input (Advanced)
6. GLFW Input Examples
7. Debugging Code

---

## 1. Basic Message Pump Examples

### 1.1 Simple Blocking Message Pump

```cpp
#include <windows.h>
#include <stdio.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main() {
    const wchar_t CLASS_NAME[] = L"MyWindowClass";
    const wchar_t WINDOW_TITLE[] = L"Input Test Window";

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"RegisterClass failed", L"Error", MB_OK);
        return 1;
    }

    // Create window
    HWND hwnd = CreateWindowEx(
        0,                                          // Extended style
        CLASS_NAME,                                 // Class name
        WINDOW_TITLE,                               // Window title
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,          // Style - CRITICAL: include WS_VISIBLE
        CW_USEDEFAULT, CW_USEDEFAULT,              // Position
        CW_USEDEFAULT, CW_USEDEFAULT,              // Size
        NULL,                                       // Parent window
        NULL,                                       // Menu
        GetModuleHandle(NULL),                      // Instance
        NULL                                        // Extra data
    );

    if (!hwnd) {
        MessageBox(NULL, L"CreateWindowEx failed", L"Error", MB_OK);
        return 1;
    }

    // Ensure window has focus
    SetFocus(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    printf("Window created. HWND = %p\n", hwnd);
    printf("Focus test: GetFocus() == hwnd ? %s\n",
           GetFocus() == hwnd ? "YES" : "NO");

    // Standard blocking message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        printf("Message: 0x%04X, wParam: 0x%p, lParam: 0x%p\n",
               msg.message, msg.wParam, msg.lParam);

        TranslateMessage(&msg);  // CRITICAL: Convert virtual keys to characters
        DispatchMessage(&msg);   // CRITICAL: Send to window procedure
    }

    printf("Program ending\n");
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            printf(">>> WM_KEYDOWN: VK = %d (0x%X)\n", (int)wParam, (int)wParam);
            return 0;

        case WM_KEYUP:
            printf(">>> WM_KEYUP: VK = %d (0x%X)\n", (int)wParam, (int)wParam);
            return 0;

        case WM_CHAR:
            printf(">>> WM_CHAR: '%c' (0x%X)\n", (char)wParam, (int)wParam);
            return 0;

        case WM_LBUTTONDOWN:
            printf(">>> WM_LBUTTONDOWN\n");
            return 0;

        case WM_LBUTTONUP:
            printf(">>> WM_LBUTTONUP\n");
            return 0;

        case WM_MOUSEMOVE:
            {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                printf(">>> WM_MOUSEMOVE: (%d, %d)\n", x, y);
            }
            return 0;

        case WM_KILLFOCUS:
            printf(">>> WM_KILLFOCUS - Input DISABLED\n");
            return 0;

        case WM_SETFOCUS:
            printf(">>> WM_SETFOCUS - Input ENABLED\n");
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
```

### 1.2 Non-Blocking Message Pump (Game Loop Pattern)

```cpp
#include <windows.h>
#include <stdio.h>
#include <chrono>

bool gameRunning = true;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void GameLoop(HWND hwnd) {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (gameRunning) {
        // Non-blocking message pump
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                gameRunning = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastTime
        );
        float deltaTime = duration.count() / 1000.0f;
        lastTime = currentTime;

        // Game update
        printf("Game loop: FPS = %d\n", (int)(1.0f / deltaTime));

        // Game render would go here
        Sleep(16);  // Target ~60 FPS
    }
}

int main() {
    const wchar_t CLASS_NAME[] = L"GameWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Game Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    SetFocus(hwnd);

    GameLoop(hwnd);

    DestroyWindow(hwnd);
    UnregisterClass(CLASS_NAME, GetModuleHandle(NULL));

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            gameRunning = false;
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
```

---

## 2. Window Procedure Input Handlers

### 2.1 Comprehensive Keyboard Handler

```cpp
case WM_KEYDOWN:
    {
        int vkCode = (int)wParam;
        int scanCode = (lParam >> 16) & 0xFF;
        int isExtended = (lParam >> 24) & 0x01;
        int previousKeyState = (lParam >> 30) & 0x01;
        int transitionState = (lParam >> 31) & 0x01;
        int repeatCount = lParam & 0xFFFF;

        printf("Key: VK=%d (0x%X), Scan=%d, Extended=%d\n",
               vkCode, vkCode, scanCode, isExtended);
        printf("  Repeat Count: %d\n", repeatCount);
        printf("  Previous State: %d (1=was down)\n", previousKeyState);
        printf("  Transition: %d (0=pressed, 1=released)\n", transitionState);

        // Check if this is the first press (not auto-repeat)
        if (!previousKeyState) {
            // Key just pressed for the first time
            printf("  -> KEY FIRST PRESSED\n");
        } else {
            // Key auto-repeat
            printf("  -> KEY AUTO-REPEAT\n");
        }

        // Handle specific keys
        switch (vkCode) {
            case VK_ESCAPE:
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;

            case VK_SPACE:
                // Only fire if not auto-repeat
                if (!previousKeyState) {
                    printf("SPACEBAR PRESSED\n");
                    // Fire weapon, jump, etc.
                }
                break;

            case 'W':
            case 'w':
                isMovingForward = true;
                break;

            case 'S':
            case 's':
                isMovingBackward = true;
                break;

            case 'A':
            case 'a':
                isMovingLeft = true;
                break;

            case 'D':
            case 'd':
                isMovingRight = true;
                break;

            case VK_SHIFT:
                isRunning = true;
                break;
        }

        return 0;
    }

case WM_KEYUP:
    {
        int vkCode = (int)wParam;

        printf("Key Released: VK=%d\n", vkCode);

        switch (vkCode) {
            case 'W':
            case 'w':
                isMovingForward = false;
                break;

            case 'S':
            case 's':
                isMovingBackward = false;
                break;

            case 'A':
            case 'a':
                isMovingLeft = false;
                break;

            case 'D':
            case 'd':
                isMovingRight = false;
                break;

            case VK_SHIFT:
                isRunning = false;
                break;
        }

        return 0;
    }

case WM_CHAR:
    {
        wchar_t ch = (wchar_t)wParam;
        printf("Character: '%c' (0x%X)\n", (char)ch, (int)ch);

        // Handle text input
        if (ch >= 32 && ch < 127) {
            // Printable ASCII
            textBuffer += (char)ch;
        }

        return 0;
    }
```

### 2.2 Comprehensive Mouse Handler

```cpp
#include <windowsx.h>  // For GET_X_LPARAM and GET_Y_LPARAM

case WM_LBUTTONDOWN:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        bool ctrlPressed = (wParam & MK_CONTROL) != 0;
        bool shiftPressed = (wParam & MK_SHIFT) != 0;

        printf("Left Click at (%d, %d)\n", xPos, yPos);
        printf("  Ctrl: %s, Shift: %s\n",
               ctrlPressed ? "YES" : "NO",
               shiftPressed ? "YES" : "NO");

        mouseLeftPressed = true;

        // Optional: Capture mouse while button is held
        SetCapture(hwnd);

        return 0;
    }

case WM_LBUTTONUP:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        printf("Left Click Released at (%d, %d)\n", xPos, yPos);

        mouseLeftPressed = false;

        // Release mouse capture
        ReleaseCapture();

        return 0;
    }

case WM_RBUTTONDOWN:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        printf("Right Click at (%d, %d)\n", xPos, yPos);

        mouseRightPressed = true;
        SetCapture(hwnd);

        return 0;
    }

case WM_RBUTTONUP:
    {
        printf("Right Click Released\n");
        mouseRightPressed = false;
        ReleaseCapture();

        return 0;
    }

case WM_MOUSEMOVE:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        // Only log every 10th event to reduce spam
        static int moveCount = 0;
        if (++moveCount % 10 == 0) {
            printf("Mouse at (%d, %d)\n", xPos, yPos);
        }

        // Calculate delta from last position
        static int lastX = xPos;
        static int lastY = yPos;

        int deltaX = xPos - lastX;
        int deltaY = yPos - lastY;

        lastX = xPos;
        lastY = yPos;

        // Use delta for camera control
        if (mouseLeftPressed) {
            camera.rotateYaw(deltaX * 0.1f);
            camera.rotatePitch(deltaY * 0.1f);
        }

        return 0;
    }

case WM_MOUSEWHEEL:
    {
        int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        printf("Scroll Wheel: Delta=%d (positive=up, negative=down)\n", wheelDelta);

        if (wheelDelta > 0) {
            // Scroll up
            camera.zoom(-1.0f);
        } else {
            // Scroll down
            camera.zoom(1.0f);
        }

        return 0;
    }
```

### 2.3 Focus Management Handler

```cpp
case WM_KILLFOCUS:
    {
        printf("[FOCUS] Lost focus\n");
        inputEnabled = false;

        // Reset all key states
        isMovingForward = false;
        isMovingBackward = false;
        isMovingLeft = false;
        isMovingRight = false;
        mouseLeftPressed = false;
        mouseRightPressed = false;

        // Pause game if needed
        gamePaused = true;

        return 0;
    }

case WM_SETFOCUS:
    {
        printf("[FOCUS] Gained focus\n");
        inputEnabled = true;

        // Resume game if needed
        // gamePaused = false;  // Optional

        return 0;
    }

case WM_ACTIVATE:
    {
        int activeState = LOWORD(wParam);
        HWND prevWindow = (HWND)lParam;

        switch (activeState) {
            case WA_INACTIVE:
                printf("[ACTIVATE] Window deactivated\n");
                inputEnabled = false;
                gamePaused = true;
                break;

            case WA_ACTIVE:
                printf("[ACTIVATE] Window activated (not by click)\n");
                inputEnabled = true;
                SetFocus(hwnd);
                break;

            case WA_CLICKACTIVE:
                printf("[ACTIVATE] Window activated by click\n");
                inputEnabled = true;
                break;
        }

        return 0;
    }
```

---

## 3. Input State Tracking

### 3.1 Simple Key State Tracker

```cpp
class KeyboardState {
private:
    bool keys[256];
    bool previousKeys[256];

public:
    KeyboardState() {
        memset(keys, 0, sizeof(keys));
        memset(previousKeys, 0, sizeof(previousKeys));
    }

    void Update() {
        // Copy current state to previous
        memcpy(previousKeys, keys, sizeof(keys));

        // Update current state
        for (int i = 0; i < 256; i++) {
            keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
        }
    }

    bool IsPressed(int vkCode) const {
        return keys[vkCode];
    }

    bool JustPressed(int vkCode) const {
        return keys[vkCode] && !previousKeys[vkCode];
    }

    bool JustReleased(int vkCode) const {
        return !keys[vkCode] && previousKeys[vkCode];
    }

    bool IsHeld(int vkCode) const {
        return keys[vkCode] && previousKeys[vkCode];
    }
};

// Usage:
KeyboardState keyboard;

void UpdateGame() {
    keyboard.Update();

    if (keyboard.JustPressed(VK_SPACE)) {
        // Space just pressed (once)
        Jump();
    }

    if (keyboard.IsPressed('W')) {
        // W is held (every frame)
        MoveForward();
    }

    if (keyboard.JustReleased('W')) {
        // W just released (once)
        StopMoving();
    }
}
```

### 3.2 Mouse State Tracker

```cpp
class MouseState {
public:
    int x, y;
    int deltaX, deltaY;
    bool leftPressed;
    bool rightPressed;
    bool middlePressed;
    int wheelDelta;

    MouseState() :
        x(0), y(0), deltaX(0), deltaY(0),
        leftPressed(false), rightPressed(false), middlePressed(false),
        wheelDelta(0) {}

    void Update() {
        // Wheel delta is per-frame, reset it
        wheelDelta = 0;

        // Get current mouse position
        POINT pt;
        GetCursorPos(&pt);

        int newX = pt.x;
        int newY = pt.y;

        deltaX = newX - x;
        deltaY = newY - y;

        x = newX;
        y = newY;
    }

    void ConvertToClientCoords(HWND hwnd) {
        POINT pt = { x, y };
        ScreenToClient(hwnd, &pt);
        x = pt.x;
        y = pt.y;
    }
};

// Usage:
MouseState mouse;

case WM_MOUSEMOVE:
    {
        // Already handled by Update() when using GetCursorPos
        return 0;
    }

case WM_LBUTTONDOWN:
    mouse.leftPressed = true;
    SetCapture(hwnd);
    return 0;

case WM_LBUTTONUP:
    mouse.leftPressed = false;
    ReleaseCapture();
    return 0;

case WM_MOUSEWHEEL:
    mouse.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    return 0;

void UpdateGame() {
    mouse.Update();

    // Camera control
    if (mouse.leftPressed) {
        camera.rotateYaw(mouse.deltaX * 0.1f);
        camera.rotatePitch(mouse.deltaY * 0.1f);
    }

    // Zoom
    if (mouse.wheelDelta > 0) {
        camera.zoom(-1.0f);
    } else if (mouse.wheelDelta < 0) {
        camera.zoom(1.0f);
    }
}
```

---

## 4. GetAsyncKeyState Examples

### 4.1 Basic Polling

```cpp
void UpdateInput() {
    // Poll keyboard
    if (GetAsyncKeyState(VK_W) & 0x8000) {
        camera.moveForward(deltaTime);
    }
    if (GetAsyncKeyState(VK_S) & 0x8000) {
        camera.moveBackward(deltaTime);
    }
    if (GetAsyncKeyState(VK_A) & 0x8000) {
        camera.moveLeft(deltaTime);
    }
    if (GetAsyncKeyState(VK_D) & 0x8000) {
        camera.moveRight(deltaTime);
    }

    // Shift for sprint
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        camera.speed *= 2.0f;
    }
}
```

### 4.2 Discrete Action Detection with GetAsyncKeyState

```cpp
class DiscreteAction {
private:
    int vkCode;
    bool wasPressed;

public:
    DiscreteAction(int code) : vkCode(code), wasPressed(false) {}

    bool JustPressed() {
        bool isPressed = (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        bool result = isPressed && !wasPressed;
        wasPressed = isPressed;
        return result;
    }

    bool IsPressed() const {
        return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
    }
};

// Usage:
DiscreteAction fireAction(VK_SPACE);
DiscreteAction jumpAction('J');

void UpdateGame() {
    if (fireAction.JustPressed()) {
        // Fire weapon once
        weapon.fire();
    }

    if (jumpAction.JustPressed()) {
        // Jump once
        character.jump();
    }

    if (fireAction.IsPressed()) {
        // Weapon held (for charging)
        weapon.charge(deltaTime);
    }
}
```

### 4.3 Combo Detection

```cpp
class ComboTracker {
private:
    static const int MAX_COMBO_KEYS = 4;
    int comboKeys[MAX_COMBO_KEYS];
    int comboCount;
    float timeSinceLastKey;
    float comboTimeout;

public:
    ComboTracker() : comboCount(0), timeSinceLastKey(0), comboTimeout(1.0f) {}

    void SetCombo(int vk1, int vk2, int vk3 = -1, int vk4 = -1) {
        comboKeys[0] = vk1;
        comboKeys[1] = vk2;
        comboKeys[2] = vk3;
        comboKeys[3] = vk4;
    }

    bool CheckCombo(int currentVkCode) {
        // Check if current key is next in combo
        if (comboCount < MAX_COMBO_KEYS && comboKeys[comboCount] == currentVkCode) {
            comboCount++;
            timeSinceLastKey = 0;

            if (comboCount >= MAX_COMBO_KEYS) {
                // Combo complete!
                return true;
            }
        } else {
            // Wrong key, reset combo
            comboCount = 0;
            timeSinceLastKey = 0;
        }

        return false;
    }

    void Update(float deltaTime) {
        timeSinceLastKey += deltaTime;

        // Reset if timeout exceeded
        if (timeSinceLastKey > comboTimeout) {
            comboCount = 0;
        }
    }
};
```

---

## 5. Raw Input (Advanced)

### 5.1 Raw Input Registration and Handler

```cpp
void RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid[2];

    // Register keyboard
    rid[0].usUsagePage = 0x01;          // Generic desktop controls
    rid[0].usUsage = 0x06;              // Keyboard
    rid[0].dwFlags = RIDEV_INPUTSINK;   // Get messages even if not focused
    rid[0].hwndTarget = hwnd;

    // Register mouse
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x02;              // Mouse
    rid[1].dwFlags = RIDEV_INPUTSINK;
    rid[1].hwndTarget = hwnd;

    if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0]))) {
        printf("Failed to register raw input devices\n");
    } else {
        printf("Raw input registered\n");
    }
}

case WM_INPUT:
    {
        UINT dwSize;
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
                           sizeof(RAWINPUTHEADER)) == -1) {
            break;
        }

        LPRAWINPUT pRawInput = (LPRAWINPUT)malloc(dwSize);
        if (!pRawInput) break;

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &dwSize,
                           sizeof(RAWINPUTHEADER)) == dwSize) {

            if (pRawInput->header.dwType == RIM_TYPEKEYBOARD) {
                RAWKEYBOARD& kb = pRawInput->data.keyboard;

                printf("Raw Keyboard: VK=%d, SC=%d, Flags=%d\n",
                       kb.VKey, kb.MakeCode, kb.Flags);

                // Flags: RI_KEY_BREAK for release, 0 for press

            } else if (pRawInput->header.dwType == RIM_TYPEMOUSE) {
                RAWMOUSE& m = pRawInput->data.mouse;

                printf("Raw Mouse: RelX=%d, RelY=%d, Buttons=%d\n",
                       m.lLastX, m.lLastY, m.usButtonFlags);

                // usButtonFlags: RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, etc.
            }
        }

        free(pRawInput);
        return 0;
    }
```

---

## 6. GLFW Input Examples

### 6.1 Polling Input (Like Your Code)

```cpp
// In your main loop
void processInput(GLFWwindow* window) {
    // Continuous movement (polling)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard(RIGHT, deltaTime);
    }

    // Discrete actions with state tracking
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spacePressed) {
            // Fire once per press
            weapon.fire();
            spacePressed = true;
        }
    } else {
        spacePressed = false;
    }

    // Exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// Main loop
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();  // CRITICAL - without this, no callbacks fire
    processInput(window);

    // Update/render...

    glfwSwapBuffers(window);
}
```

### 6.2 Event-Based Input (Callbacks)

```cpp
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        printf("Key pressed: %d\n", key);

        if (key == GLFW_KEY_SPACE) {
            // Fire once (event-driven)
            weapon.fire();
        }
    } else if (action == GLFW_RELEASE) {
        printf("Key released: %d\n", key);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = xpos;
    static double lastY = ypos;

    double deltaX = xpos - lastX;
    double deltaY = lastY - ypos;  // Note: Y is inverted

    lastX = xpos;
    lastY = ypos;

    // Camera control
    camera.processMouseMovement((float)deltaX, (float)deltaY);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Camera zoom
    if (yoffset > 0) {
        camera.zoom(-1.0f);
    } else {
        camera.zoom(1.0f);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Left click (event-driven)
        weapon.fire();
    }
}

// Register callbacks
glfwSetKeyCallback(window, key_callback);
glfwSetCursorPosCallback(window, mouse_callback);
glfwSetScrollCallback(window, scroll_callback);
glfwSetMouseButtonCallback(window, mouse_button_callback);
```

---

## 7. Debugging Code

### 7.1 Input Diagnostics

```cpp
void PrintInputDiagnostics(HWND hwnd) {
    printf("\n=== INPUT DIAGNOSTICS ===\n");

    // Window focus
    HWND focusedWindow = GetFocus();
    printf("Window focused: %s\n",
           focusedWindow == hwnd ? "YES" : "NO");

    // Window visibility
    bool isVisible = IsWindowVisible(hwnd);
    printf("Window visible: %s\n", isVisible ? "YES" : "NO");

    // Window styles
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

    printf("Style flags:\n");
    if (style & WS_VISIBLE) printf("  + WS_VISIBLE\n");
    if (style & WS_DISABLED) printf("  - WS_DISABLED (PROBLEM!)\n");
    if (style & WS_OVERLAPPEDWINDOW) printf("  + WS_OVERLAPPEDWINDOW\n");

    printf("Extended style flags:\n");
    if (exStyle & WS_EX_TRANSPARENT) printf("  - WS_EX_TRANSPARENT (PROBLEM!)\n");
    if (exStyle & WS_EX_NOACTIVATE) printf("  - WS_EX_NOACTIVATE (PROBLEM!)\n");
    if (exStyle & WS_EX_APPWINDOW) printf("  + WS_EX_APPWINDOW\n");

    // Cursor
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    printf("Cursor position: (%d, %d)\n", cursorPos.x, cursorPos.y);

    ScreenToClient(hwnd, &cursorPos);
    printf("Cursor in client area: (%d, %d)\n", cursorPos.x, cursorPos.y);

    // Key states
    printf("Key states:\n");
    printf("  W: %s\n", (GetAsyncKeyState('W') & 0x8000) ? "DOWN" : "UP");
    printf("  A: %s\n", (GetAsyncKeyState('A') & 0x8000) ? "DOWN" : "UP");
    printf("  S: %s\n", (GetAsyncKeyState('S') & 0x8000) ? "DOWN" : "UP");
    printf("  D: %s\n", (GetAsyncKeyState('D') & 0x8000) ? "DOWN" : "UP");
    printf("  Space: %s\n", (GetAsyncKeyState(VK_SPACE) & 0x8000) ? "DOWN" : "UP");
    printf("  Shift: %s\n", (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? "DOWN" : "UP");
    printf("  Ctrl: %s\n", (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? "DOWN" : "UP");

    printf("======================\n\n");
}

// Call in main loop or on demand
if (diagnosticsEnabled) {
    PrintInputDiagnostics(hwnd);
    diagnosticsEnabled = false;  // Only print once per request
}
```

### 7.2 GLFW Input Diagnostics

```cpp
void PrintGLFWDiagnostics(GLFWwindow* window) {
    printf("\n=== GLFW INPUT DIAGNOSTICS ===\n");

    // Window properties
    printf("Window created: %s\n", window ? "YES" : "NO");
    printf("Window focused: %s\n", glfwGetWindowAttrib(window, GLFW_FOCUSED) ? "YES" : "NO");

    // Input modes
    int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
    printf("Cursor mode: %s\n",
           cursorMode == GLFW_CURSOR_NORMAL ? "NORMAL" :
           cursorMode == GLFW_CURSOR_HIDDEN ? "HIDDEN" :
           cursorMode == GLFW_CURSOR_DISABLED ? "DISABLED" :
           "UNKNOWN");

    // Key states
    printf("Key states:\n");
    printf("  W: %s\n", glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  A: %s\n", glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  S: %s\n", glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  D: %s\n", glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  Space: %s\n", glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  Escape: %s\n", glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ? "PRESSED" : "RELEASED");

    // Mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    printf("Mouse position: (%.1f, %.1f)\n", xpos, ypos);

    // Mouse buttons
    printf("Mouse buttons:\n");
    printf("  Left: %s\n", glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  Right: %s\n", glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ? "PRESSED" : "RELEASED");
    printf("  Middle: %s\n", glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ? "PRESSED" : "RELEASED");

    printf("==============================\n\n");
}
```

---

## Summary

These code examples cover:

1. **Message pumps** - Both blocking and non-blocking patterns
2. **Input handlers** - For keyboard, mouse, and focus events
3. **State tracking** - How to detect single presses vs. continuous holds
4. **GetAsyncKeyState** - Direct polling for continuous input
5. **Raw input** - Low-level input for special cases
6. **GLFW** - High-level abstractions (what you're using)
7. **Debugging** - Tools to diagnose input problems

For your project using GLFW, the critical code is:
- `glfwPollEvents()` every frame
- Callbacks registered with `glfwSetXCallback()`
- State tracking for discrete actions (like buttons)
