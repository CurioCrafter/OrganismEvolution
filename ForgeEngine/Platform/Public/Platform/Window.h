#pragma once

// Forge Engine - Window System
// Platform-independent window management

#include "Core/CoreMinimal.h"
#include "Math/Vector.h"
#include <string>

namespace Forge::Platform
{
    // ========================================================================
    // Window Configuration
    // ========================================================================

    enum class WindowMode : u8
    {
        Windowed,           // Standard window with decorations
        Borderless,         // Borderless window
        Fullscreen,         // Exclusive fullscreen
        BorderlessFullscreen // Borderless window at screen resolution
    };

    struct WindowConfig
    {
        std::string title = "Forge Engine";
        u32 width = 1280;
        u32 height = 720;
        i32 x = -1;  // -1 = centered
        i32 y = -1;  // -1 = centered
        WindowMode mode = WindowMode::Windowed;
        bool resizable = true;
        bool visible = true;
        bool focused = true;
        bool vsync = true;
    };

    // ========================================================================
    // Window Events
    // ========================================================================

    enum class WindowEventType : u8
    {
        None,
        Close,          // Window close requested
        Resize,         // Window resized
        Move,           // Window moved
        Focus,          // Window gained focus
        Blur,           // Window lost focus
        Minimize,       // Window minimized
        Maximize,       // Window maximized
        Restore,        // Window restored from minimize/maximize
        DPIChange,      // DPI changed (e.g., moved to different monitor)
    };

    struct WindowEvent
    {
        WindowEventType type = WindowEventType::None;

        union
        {
            struct { u32 width, height; } resize;
            struct { i32 x, y; } move;
            struct { f32 scale; } dpi;
        };
    };

    // ========================================================================
    // Input State (Basic - will be expanded in Input module)
    // ========================================================================

    /// Mouse button
    enum class MouseButton : u8
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,
        X2 = 4,
        COUNT
    };

    /// Key codes (subset - full list in Input module)
    enum class KeyCode : u16
    {
        Unknown = 0,

        // Letters
        A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H',
        I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P',
        Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X',
        Y = 'Y', Z = 'Z',

        // Numbers
        Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
        Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',

        // Function keys
        F1 = 256, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        // Control keys
        Escape = 300, Tab, CapsLock, LeftShift, LeftControl, LeftAlt, LeftSuper,
        RightShift, RightControl, RightAlt, RightSuper,
        Space, Enter, Backspace, Delete, Insert,

        // Navigation
        Left, Right, Up, Down,
        Home, End, PageUp, PageDown,

        // Misc
        PrintScreen, ScrollLock, Pause,

        COUNT
    };

    // ========================================================================
    // Window Interface
    // ========================================================================

    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        // ====================================================================
        // Lifecycle
        // ====================================================================

        /// Process window messages and return false if window should close
        virtual bool PollEvents() = 0;

        /// Check if window is still open
        [[nodiscard]] virtual bool IsOpen() const = 0;

        /// Check if window should close (alias for !IsOpen())
        [[nodiscard]] bool ShouldClose() const { return !IsOpen(); }

        /// Request window close
        virtual void Close() = 0;

        // ====================================================================
        // Properties
        // ====================================================================

        /// Get window title
        [[nodiscard]] virtual StringView GetTitle() const = 0;

        /// Set window title
        virtual void SetTitle(StringView title) = 0;

        /// Get window size (client area)
        [[nodiscard]] virtual Math::Vec2 GetSize() const = 0;

        /// Get window size (output parameters)
        virtual void GetSize(u32& outWidth, u32& outHeight) const
        {
            outWidth = GetWidth();
            outHeight = GetHeight();
        }

        /// Get window width
        [[nodiscard]] virtual u32 GetWidth() const = 0;

        /// Get window height
        [[nodiscard]] virtual u32 GetHeight() const = 0;

        /// Set window size
        virtual void SetSize(u32 width, u32 height) = 0;

        /// Get window position
        [[nodiscard]] virtual Math::Vec2 GetPosition() const = 0;

        /// Set window position
        virtual void SetPosition(i32 x, i32 y) = 0;

        /// Get window mode
        [[nodiscard]] virtual WindowMode GetMode() const = 0;

        /// Set window mode
        virtual void SetMode(WindowMode mode) = 0;

        /// Check if window is minimized
        [[nodiscard]] virtual bool IsMinimized() const = 0;

        /// Check if window is maximized
        [[nodiscard]] virtual bool IsMaximized() const = 0;

        /// Check if window has focus
        [[nodiscard]] virtual bool IsFocused() const = 0;

        // ====================================================================
        // Display
        // ====================================================================

        /// Show window
        virtual void Show() = 0;

        /// Hide window
        virtual void Hide() = 0;

        /// Minimize window
        virtual void Minimize() = 0;

        /// Maximize window
        virtual void Maximize() = 0;

        /// Restore window from minimize/maximize
        virtual void Restore() = 0;

        /// Focus window
        virtual void Focus() = 0;

        /// Set VSync
        virtual void SetVSync(bool enabled) = 0;

        /// Get DPI scale factor
        [[nodiscard]] virtual f32 GetDPIScale() const = 0;

        // ====================================================================
        // Native Handle
        // ====================================================================

        /// Get native window handle (HWND on Windows, etc.)
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        // ====================================================================
        // Event Callback
        // ====================================================================

        using EventCallback = Function<void(const WindowEvent&)>;

        /// Set event callback
        virtual void SetEventCallback(EventCallback callback) = 0;

        // ====================================================================
        // Input
        // ====================================================================

        /// Check if a key is currently pressed
        [[nodiscard]] virtual bool IsKeyDown(KeyCode key) const = 0;

        /// Check if a key was just pressed this frame
        [[nodiscard]] virtual bool IsKeyPressed(KeyCode key) const = 0;

        /// Check if a mouse button is currently pressed
        [[nodiscard]] virtual bool IsMouseButtonDown(MouseButton button) const = 0;

        /// Get mouse position relative to window
        [[nodiscard]] virtual Math::Vec2 GetMousePosition() const = 0;

        /// Get mouse delta since last frame
        [[nodiscard]] virtual Math::Vec2 GetMouseDelta() const = 0;

        /// Set mouse cursor visibility
        virtual void SetCursorVisible(bool visible) = 0;

        /// Set mouse cursor locked to window center (for FPS-style controls)
        virtual void SetCursorLocked(bool locked) = 0;

        /// Check if cursor is locked
        [[nodiscard]] virtual bool IsCursorLocked() const = 0;

        // ====================================================================
        // Factory
        // ====================================================================

        /// Create a window
        static UniquePtr<IWindow> Create(const WindowConfig& config = {});
    };

} // namespace Forge::Platform
