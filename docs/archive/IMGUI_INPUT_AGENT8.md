# ImGui Input Capture Fix - Agent 8

## Problem Description

Clicking on the ImGui debug panel (F3) caused clicks to "pass through" to the game, triggering mouse capture mode even when the user was trying to interact with UI controls like sliders, buttons, and input fields.

## Root Cause Analysis

The application had **two separate input handling systems** that were not coordinated:

### 1. Window Message System (WindowsWindow.cpp)
The `WindowsWindow::HandleMessage()` function correctly integrated with ImGui:
```cpp
// Only process ImGui if it's initialized (prevents crash on early messages)
if (ImGui::GetCurrentContext() != nullptr)
{
    ImGui_ImplWin32_WndProcHandler(m_hwnd, msg, wParam, lParam);

    // Only let ImGui block input if it wants capture
    ImGuiIO& io = ImGui::GetIO();
    if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) && io.WantCaptureMouse)
        return true;
    if ((msg >= WM_KEYFIRST && msg <= WM_KEYLAST) && io.WantCaptureKeyboard)
        return true;
}
```

This was working correctly - ImGui received all input events and could block them from reaching the game's window message handlers.

### 2. Direct Hardware Polling (main.cpp HandleInput)
The `HandleInput()` function used `GetAsyncKeyState()` to directly poll hardware state:
```cpp
// This bypasses the window message system entirely!
bool leftPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

// Click to capture - NO ImGui check!
if (leftPressed && !s_wasLeftPressed && !s_mouseCaptured)
{
    s_mouseCaptured = true;
    // ... capture mouse ...
}
```

The problem: `GetAsyncKeyState()` reads directly from the keyboard/mouse hardware state, completely bypassing the Windows message system. This meant ImGui's input blocking had no effect on this code path.

## How ImGui Input Capture Works

ImGui uses two flags in `ImGuiIO` to communicate whether it wants to capture input:

- **`io.WantCaptureMouse`**: Set to `true` when the mouse is hovering over an ImGui window, or when an ImGui widget is being dragged/interacted with
- **`io.WantCaptureKeyboard`**: Set to `true` when an ImGui text input field has focus, or a keyboard-navigable widget is active

Applications should check these flags and skip their own input processing when ImGui wants capture. This creates the proper "layered" UI behavior where the topmost layer (ImGui) can intercept input.

## The Fix

### Change 1: Mouse Click Check (line ~4153-4168)

Added ImGui capture check before processing mouse clicks:

```cpp
// Check if ImGui wants to capture input (prevents click-through on UI panels)
bool imguiWantsMouse = false;
bool imguiWantsKeyboard = false;
if (ImGui::GetCurrentContext() != nullptr)
{
    ImGuiIO& io = ImGui::GetIO();
    imguiWantsMouse = io.WantCaptureMouse;
    imguiWantsKeyboard = io.WantCaptureKeyboard;
}

// ... later ...

// Click to capture (only if ImGui doesn't want the mouse)
if (leftPressed && !s_wasLeftPressed && !s_mouseCaptured && !imguiWantsMouse)
{
    // ... capture mouse ...
}
```

### Change 2: Keyboard Controls (line ~4216-4250)

Protected keyboard controls from interfering with ImGui text input:

```cpp
// P to toggle pause (only when ImGui doesn't want keyboard)
if (pPressed && !s_wasPPressed && !imguiWantsKeyboard)
{
    m_renderer.m_simulationPaused = !m_renderer.m_simulationPaused;
}

// Keyboard movement (only when ImGui doesn't want keyboard input)
if (!imguiWantsKeyboard)
{
    // WASD, Q/E, Space, C, Ctrl movement keys
}
```

Note: F3 (toggle debug panel) is intentionally NOT blocked by `imguiWantsKeyboard` because:
1. It's a function key, not a character key that would conflict with text input
2. Users need to be able to close the debug panel even when it has keyboard focus

## Files Modified

- **`src/main.cpp`**: Added ImGui capture checks in `HandleInput()` function

## Testing Instructions

1. Build and run the simulation
2. Press F3 to open the debug panel
3. **Test Mouse Capture**:
   - Click on sliders in the debug panel - should NOT capture mouse
   - Click on empty space outside the panel - SHOULD capture mouse
   - Drag sliders - should work smoothly without triggering camera movement
4. **Test Keyboard Input**:
   - With debug panel open, press P - should NOT toggle pause (if panel has focus)
   - Click outside panel to remove focus, press P - SHOULD toggle pause
   - WASD keys should not move camera when interacting with UI
5. **Test ESC Release**:
   - Capture mouse (click outside panel), then press ESC
   - Should release mouse capture and show cursor
   - F3 should still toggle the debug panel

## Technical Notes

### Why GetAsyncKeyState Bypasses ImGui

`GetAsyncKeyState()` queries the hardware state directly through the Windows input system. It does not go through the message queue, so:
- It's instantaneous (no message pump delay)
- It's not affected by message filtering
- It bypasses any application-level input blocking

This is useful for game controls that need immediate response, but requires manual coordination with UI systems like ImGui.

### Alternative Approaches

1. **Use Window Messages Only**: Could rewrite input handling to use only WM_LBUTTONDOWN etc, but this adds input latency and complexity
2. **Raw Input API**: Could use Raw Input, but same issue - need to check ImGui capture manually
3. **Input Abstraction Layer**: Could create a unified input system that checks ImGui capture first, then dispatches to game or UI. This is more architecturally clean but requires significant refactoring.

The chosen fix is minimal and directly addresses the problem without major architectural changes.

## Related Files

- `ForgeEngine/Platform/Private/Windows/WindowsWindow.cpp`: Contains the WndProc handler with existing ImGui integration
- `external/imgui/imgui_impl_win32.h`: ImGui Win32 backend that processes Windows messages
