#include "Platform/Window.h"

#if FORGE_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <Windows.h>
#include <windowsx.h>

// ImGui Win32 backend for input handling
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Undefine Windows macros that conflict with our method names
#ifdef IsMinimized
    #undef IsMinimized
#endif
#ifdef IsMaximized
    #undef IsMaximized
#endif

namespace Forge::Platform
{
    // ========================================================================
    // Windows Window Implementation
    // ========================================================================

    class WindowsWindow : public IWindow
    {
    public:
        WindowsWindow(const WindowConfig& config);
        ~WindowsWindow() override;

        bool PollEvents() override;
        bool IsOpen() const override { return m_isOpen; }
        void Close() override { m_isOpen = false; }

        StringView GetTitle() const override { return m_title; }
        void SetTitle(StringView title) override;

        Math::Vec2 GetSize() const override { return Math::Vec2(static_cast<f32>(m_width), static_cast<f32>(m_height)); }
        u32 GetWidth() const override { return m_width; }
        u32 GetHeight() const override { return m_height; }
        void SetSize(u32 width, u32 height) override;

        Math::Vec2 GetPosition() const override { return Math::Vec2(static_cast<f32>(m_x), static_cast<f32>(m_y)); }
        void SetPosition(i32 x, i32 y) override;

        WindowMode GetMode() const override { return m_mode; }
        void SetMode(WindowMode mode) override;

        bool IsMinimized() const override { return m_minimized; }
        bool IsMaximized() const override { return m_maximized; }
        bool IsFocused() const override { return m_focused; }

        void Show() override;
        void Hide() override;
        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void Focus() override;

        void SetVSync(bool enabled) override { m_vsync = enabled; }
        f32 GetDPIScale() const override { return m_dpiScale; }

        void* GetNativeHandle() const override { return m_hwnd; }

        void SetEventCallback(EventCallback callback) override { m_eventCallback = std::move(callback); }

        // Input
        bool IsKeyDown(KeyCode key) const override;
        bool IsKeyPressed(KeyCode key) const override;
        bool IsMouseButtonDown(MouseButton button) const override;
        Math::Vec2 GetMousePosition() const override;
        Math::Vec2 GetMouseDelta() const override;
        void SetCursorVisible(bool visible) override;
        void SetCursorLocked(bool locked) override;
        bool IsCursorLocked() const override { return m_cursorLocked; }

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        void UpdateWindowStyle();
        void CenterWindow();

        HWND m_hwnd = nullptr;
        HINSTANCE m_hinstance = nullptr;

        std::string m_title;
        u32 m_width = 0;
        u32 m_height = 0;
        i32 m_x = 0;
        i32 m_y = 0;
        WindowMode m_mode = WindowMode::Windowed;

        bool m_isOpen = false;
        bool m_minimized = false;
        bool m_maximized = false;
        bool m_focused = true;
        bool m_vsync = true;
        f32 m_dpiScale = 1.0f;

        EventCallback m_eventCallback;

        // Input state
        bool m_keyState[512] = {};       // Current frame key state
        bool m_keyStatePrev[512] = {};   // Previous frame key state
        bool m_mouseButtons[5] = {};     // Mouse button state
        Math::Vec2 m_mousePos = { 0.0f, 0.0f };
        Math::Vec2 m_mouseDelta = { 0.0f, 0.0f };
        Math::Vec2 m_lastMousePos = { 0.0f, 0.0f };
        bool m_cursorVisible = true;
        bool m_cursorLocked = false;
        bool m_firstMouseMove = true;

        // For fullscreen toggle
        WINDOWPLACEMENT m_windowPlacement = { sizeof(WINDOWPLACEMENT) };

        static const wchar_t* WINDOW_CLASS_NAME;
        static bool s_windowClassRegistered;
    };

    const wchar_t* WindowsWindow::WINDOW_CLASS_NAME = L"ForgeWindowClass";
    bool WindowsWindow::s_windowClassRegistered = false;

    WindowsWindow::WindowsWindow(const WindowConfig& config)
        : m_title(config.title)
        , m_width(config.width)
        , m_height(config.height)
        , m_mode(config.mode)
        , m_vsync(config.vsync)
    {
        m_hinstance = GetModuleHandle(nullptr);

        // Register window class
        if (!s_windowClassRegistered)
        {
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = m_hinstance;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
            wc.lpszClassName = WINDOW_CLASS_NAME;
            wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

            ATOM classAtom = RegisterClassExW(&wc);
            if (classAtom == 0)
            {
                DWORD error = GetLastError();
                char msg[256];
                snprintf(msg, sizeof(msg), "Failed to register window class (error %lu)", error);
                FORGE_VERIFY_MSG(false, msg);
            }
            s_windowClassRegistered = true;
        }

        // Calculate window style
        DWORD style = WS_OVERLAPPEDWINDOW;
        DWORD exStyle = WS_EX_APPWINDOW;

        if (!config.resizable)
        {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        if (config.mode == WindowMode::Borderless || config.mode == WindowMode::BorderlessFullscreen)
        {
            style = WS_POPUP;
        }

        // Adjust window rect for client area
        RECT rect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;

        // Calculate position
        if (config.x < 0 || config.y < 0)
        {
            // Center on primary monitor
            m_x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
            m_y = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;
        }
        else
        {
            m_x = config.x;
            m_y = config.y;
        }

        // Convert title to wide string
        int titleLen = MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, nullptr, 0);
        std::wstring wideTitle(titleLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, wideTitle.data(), titleLen);

        // Create window
        m_hwnd = CreateWindowExW(
            exStyle,
            WINDOW_CLASS_NAME,
            wideTitle.c_str(),
            style | WS_VISIBLE,
            m_x, m_y,
            windowWidth, windowHeight,
            nullptr,
            nullptr,
            m_hinstance,
            this  // Pass this pointer for WM_CREATE
        );

        if (m_hwnd == nullptr)
        {
            DWORD error = GetLastError();
            char msg[256];
            snprintf(msg, sizeof(msg), "Failed to create window (error %lu)", error);
            FORGE_VERIFY_MSG(false, msg);
        }

        // Get DPI
        HDC hdc = GetDC(m_hwnd);
        m_dpiScale = static_cast<f32>(GetDeviceCaps(hdc, LOGPIXELSX)) / 96.0f;
        ReleaseDC(m_hwnd, hdc);

        // Show window
        if (config.visible)
        {
            ShowWindow(m_hwnd, config.focused ? SW_SHOW : SW_SHOWNOACTIVATE);
        }

        m_isOpen = true;
        m_focused = config.focused;

        // Handle fullscreen
        if (config.mode == WindowMode::Fullscreen || config.mode == WindowMode::BorderlessFullscreen)
        {
            SetMode(config.mode);
        }
    }

    WindowsWindow::~WindowsWindow()
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    bool WindowsWindow::PollEvents()
    {
        // Copy current key state to previous
        memcpy(m_keyStatePrev, m_keyState, sizeof(m_keyState));

        // Reset mouse delta
        m_mouseDelta = { 0.0f, 0.0f };

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_isOpen = false;
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Handle cursor lock - keep cursor centered
        if (m_cursorLocked && m_focused)
        {
            RECT rect;
            GetClientRect(m_hwnd, &rect);
            POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
            ClientToScreen(m_hwnd, &center);
            SetCursorPos(center.x, center.y);
        }

        return m_isOpen;
    }

    void WindowsWindow::SetTitle(StringView title)
    {
        m_title = std::string(title);

        int titleLen = MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, nullptr, 0);
        std::wstring wideTitle(titleLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, wideTitle.data(), titleLen);

        SetWindowTextW(m_hwnd, wideTitle.c_str());
    }

    void WindowsWindow::SetSize(u32 width, u32 height)
    {
        RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
        DWORD exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        SetWindowPos(m_hwnd, nullptr, 0, 0,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOMOVE | SWP_NOZORDER);

        m_width = width;
        m_height = height;
    }

    void WindowsWindow::SetPosition(i32 x, i32 y)
    {
        SetWindowPos(m_hwnd, nullptr, x, y, 0, 0,
                    SWP_NOSIZE | SWP_NOZORDER);
        m_x = x;
        m_y = y;
    }

    void WindowsWindow::SetMode(WindowMode mode)
    {
        if (mode == m_mode) return;

        if (mode == WindowMode::Fullscreen || mode == WindowMode::BorderlessFullscreen)
        {
            // Save current window placement
            GetWindowPlacement(m_hwnd, &m_windowPlacement);

            // Get monitor info
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

            // Set borderless style
            SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

            // Set to monitor size
            SetWindowPos(m_hwnd, HWND_TOP,
                        mi.rcMonitor.left, mi.rcMonitor.top,
                        mi.rcMonitor.right - mi.rcMonitor.left,
                        mi.rcMonitor.bottom - mi.rcMonitor.top,
                        SWP_FRAMECHANGED);
        }
        else
        {
            // Restore window style
            DWORD style = WS_OVERLAPPEDWINDOW;
            if (mode == WindowMode::Borderless)
            {
                style = WS_POPUP | WS_VISIBLE;
            }

            SetWindowLong(m_hwnd, GWL_STYLE, style);
            SetWindowPlacement(m_hwnd, &m_windowPlacement);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        m_mode = mode;
    }

    void WindowsWindow::Show()
    {
        ShowWindow(m_hwnd, SW_SHOW);
    }

    void WindowsWindow::Hide()
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }

    void WindowsWindow::Minimize()
    {
        ShowWindow(m_hwnd, SW_MINIMIZE);
    }

    void WindowsWindow::Maximize()
    {
        ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    void WindowsWindow::Restore()
    {
        ShowWindow(m_hwnd, SW_RESTORE);
    }

    void WindowsWindow::Focus()
    {
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);
    }

    LRESULT CALLBACK WindowsWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        WindowsWindow* window = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            window = static_cast<WindowsWindow*>(createStruct->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            // Must call DefWindowProc for WM_NCCREATE to allow window creation
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (window)
        {
            return window->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT WindowsWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Only process ImGui if it's initialized (prevents crash on early messages)
        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui_ImplWin32_WndProcHandler(m_hwnd, msg, wParam, lParam);
        }

        switch (msg)
        {
            case WM_CLOSE:
            {
                WindowEvent event;
                event.type = WindowEventType::Close;
                if (m_eventCallback) m_eventCallback(event);

                m_isOpen = false;
                return 0;
            }

            case WM_SIZE:
            {
                m_width = LOWORD(lParam);
                m_height = HIWORD(lParam);
                m_minimized = (wParam == SIZE_MINIMIZED);
                m_maximized = (wParam == SIZE_MAXIMIZED);

                WindowEvent event;
                if (wParam == SIZE_MINIMIZED)
                    event.type = WindowEventType::Minimize;
                else if (wParam == SIZE_MAXIMIZED)
                    event.type = WindowEventType::Maximize;
                else if (wParam == SIZE_RESTORED)
                    event.type = WindowEventType::Resize;
                else
                    event.type = WindowEventType::Resize;

                event.resize.width = m_width;
                event.resize.height = m_height;

                if (m_eventCallback) m_eventCallback(event);
                return 0;
            }

            case WM_MOVE:
            {
                m_x = static_cast<i32>(static_cast<i16>(LOWORD(lParam)));
                m_y = static_cast<i32>(static_cast<i16>(HIWORD(lParam)));

                WindowEvent event;
                event.type = WindowEventType::Move;
                event.move.x = m_x;
                event.move.y = m_y;

                if (m_eventCallback) m_eventCallback(event);
                return 0;
            }

            case WM_SETFOCUS:
            {
                m_focused = true;
                WindowEvent event;
                event.type = WindowEventType::Focus;
                if (m_eventCallback) m_eventCallback(event);
                return 0;
            }

            case WM_KILLFOCUS:
            {
                m_focused = false;
                if (m_cursorLocked) {
                    SetCursorLocked(false);
                }
                WindowEvent event;
                event.type = WindowEventType::Blur;
                if (m_eventCallback) m_eventCallback(event);
                return 0;
            }

            case WM_DPICHANGED:
            {
                m_dpiScale = static_cast<f32>(HIWORD(wParam)) / 96.0f;

                // Resize window to suggested rect
                auto* rect = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(m_hwnd, nullptr,
                            rect->left, rect->top,
                            rect->right - rect->left,
                            rect->bottom - rect->top,
                            SWP_NOZORDER);

                WindowEvent event;
                event.type = WindowEventType::DPIChange;
                event.dpi.scale = m_dpiScale;
                if (m_eventCallback) m_eventCallback(event);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1;  // Prevent flicker

            // Keyboard input
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                if (wParam < 512)
                    m_keyState[wParam] = true;
                return 0;
            }

            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                if (wParam < 512)
                    m_keyState[wParam] = false;
                return 0;
            }

            // Mouse input
            case WM_MOUSEMOVE:
            {
                f32 x = static_cast<f32>(GET_X_LPARAM(lParam));
                f32 y = static_cast<f32>(GET_Y_LPARAM(lParam));

                if (m_firstMouseMove)
                {
                    m_lastMousePos = { x, y };
                    m_firstMouseMove = false;
                }

                m_mousePos = { x, y };

                if (m_cursorLocked)
                {
                    // Calculate delta from center
                    RECT rect;
                    GetClientRect(m_hwnd, &rect);
                    f32 centerX = static_cast<f32>(rect.right - rect.left) / 2.0f;
                    f32 centerY = static_cast<f32>(rect.bottom - rect.top) / 2.0f;
                    m_mouseDelta.x += x - centerX;
                    m_mouseDelta.y += y - centerY;
                }
                else
                {
                    m_mouseDelta.x += x - m_lastMousePos.x;
                    m_mouseDelta.y += y - m_lastMousePos.y;
                }

                m_lastMousePos = { x, y };
                return 0;
            }

            case WM_LBUTTONDOWN:
                SetFocus(m_hwnd);
                m_mouseButtons[static_cast<u32>(MouseButton::Left)] = true;
                return 0;
            case WM_LBUTTONUP:
                m_mouseButtons[static_cast<u32>(MouseButton::Left)] = false;
                return 0;
            case WM_RBUTTONDOWN:
                m_mouseButtons[static_cast<u32>(MouseButton::Right)] = true;
                return 0;
            case WM_RBUTTONUP:
                m_mouseButtons[static_cast<u32>(MouseButton::Right)] = false;
                return 0;
            case WM_MBUTTONDOWN:
                m_mouseButtons[static_cast<u32>(MouseButton::Middle)] = true;
                return 0;
            case WM_MBUTTONUP:
                m_mouseButtons[static_cast<u32>(MouseButton::Middle)] = false;
                return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    // ========================================================================
    // Input
    // ========================================================================

    static int KeyCodeToVK(KeyCode key)
    {
        switch (key)
        {
            // Letters map directly to VK codes (which match ASCII)
            case KeyCode::A: return 'A';
            case KeyCode::B: return 'B';
            case KeyCode::C: return 'C';
            case KeyCode::D: return 'D';
            case KeyCode::E: return 'E';
            case KeyCode::F: return 'F';
            case KeyCode::G: return 'G';
            case KeyCode::H: return 'H';
            case KeyCode::I: return 'I';
            case KeyCode::J: return 'J';
            case KeyCode::K: return 'K';
            case KeyCode::L: return 'L';
            case KeyCode::M: return 'M';
            case KeyCode::N: return 'N';
            case KeyCode::O: return 'O';
            case KeyCode::P: return 'P';
            case KeyCode::Q: return 'Q';
            case KeyCode::R: return 'R';
            case KeyCode::S: return 'S';
            case KeyCode::T: return 'T';
            case KeyCode::U: return 'U';
            case KeyCode::V: return 'V';
            case KeyCode::W: return 'W';
            case KeyCode::X: return 'X';
            case KeyCode::Y: return 'Y';
            case KeyCode::Z: return 'Z';

            // Numbers
            case KeyCode::Num0: return '0';
            case KeyCode::Num1: return '1';
            case KeyCode::Num2: return '2';
            case KeyCode::Num3: return '3';
            case KeyCode::Num4: return '4';
            case KeyCode::Num5: return '5';
            case KeyCode::Num6: return '6';
            case KeyCode::Num7: return '7';
            case KeyCode::Num8: return '8';
            case KeyCode::Num9: return '9';

            // Function keys
            case KeyCode::F1: return VK_F1;
            case KeyCode::F2: return VK_F2;
            case KeyCode::F3: return VK_F3;
            case KeyCode::F4: return VK_F4;
            case KeyCode::F5: return VK_F5;
            case KeyCode::F6: return VK_F6;
            case KeyCode::F7: return VK_F7;
            case KeyCode::F8: return VK_F8;
            case KeyCode::F9: return VK_F9;
            case KeyCode::F10: return VK_F10;
            case KeyCode::F11: return VK_F11;
            case KeyCode::F12: return VK_F12;

            // Control keys
            case KeyCode::Escape: return VK_ESCAPE;
            case KeyCode::Tab: return VK_TAB;
            case KeyCode::CapsLock: return VK_CAPITAL;
            case KeyCode::LeftShift: return VK_LSHIFT;
            case KeyCode::LeftControl: return VK_LCONTROL;
            case KeyCode::LeftAlt: return VK_LMENU;
            case KeyCode::LeftSuper: return VK_LWIN;
            case KeyCode::RightShift: return VK_RSHIFT;
            case KeyCode::RightControl: return VK_RCONTROL;
            case KeyCode::RightAlt: return VK_RMENU;
            case KeyCode::RightSuper: return VK_RWIN;
            case KeyCode::Space: return VK_SPACE;
            case KeyCode::Enter: return VK_RETURN;
            case KeyCode::Backspace: return VK_BACK;
            case KeyCode::Delete: return VK_DELETE;
            case KeyCode::Insert: return VK_INSERT;

            // Navigation
            case KeyCode::Left: return VK_LEFT;
            case KeyCode::Right: return VK_RIGHT;
            case KeyCode::Up: return VK_UP;
            case KeyCode::Down: return VK_DOWN;
            case KeyCode::Home: return VK_HOME;
            case KeyCode::End: return VK_END;
            case KeyCode::PageUp: return VK_PRIOR;
            case KeyCode::PageDown: return VK_NEXT;

            default: return 0;
        }
    }

    bool WindowsWindow::IsKeyDown(KeyCode key) const
    {
        int vk = KeyCodeToVK(key);
        if (vk > 0 && vk < 512)
            return m_keyState[vk];
        return false;
    }

    bool WindowsWindow::IsKeyPressed(KeyCode key) const
    {
        int vk = KeyCodeToVK(key);
        if (vk > 0 && vk < 512)
            return m_keyState[vk] && !m_keyStatePrev[vk];
        return false;
    }

    bool WindowsWindow::IsMouseButtonDown(MouseButton button) const
    {
        u32 idx = static_cast<u32>(button);
        if (idx < 5)
            return m_mouseButtons[idx];
        return false;
    }

    Math::Vec2 WindowsWindow::GetMousePosition() const
    {
        return m_mousePos;
    }

    Math::Vec2 WindowsWindow::GetMouseDelta() const
    {
        return m_mouseDelta;
    }

    void WindowsWindow::SetCursorVisible(bool visible)
    {
        if (visible != m_cursorVisible)
        {
            m_cursorVisible = visible;
            ShowCursor(visible ? TRUE : FALSE);
        }
    }

    void WindowsWindow::SetCursorLocked(bool locked)
    {
        m_cursorLocked = locked;
        m_firstMouseMove = true;

        if (locked)
        {
            // Hide cursor and clip to window
            SetCursorVisible(false);
            SetCapture(m_hwnd);
            RECT rect;
            GetClientRect(m_hwnd, &rect);
            POINT topLeft = { rect.left, rect.top };
            POINT bottomRight = { rect.right, rect.bottom };
            ClientToScreen(m_hwnd, &topLeft);
            ClientToScreen(m_hwnd, &bottomRight);
            rect.left = topLeft.x;
            rect.top = topLeft.y;
            rect.right = bottomRight.x;
            rect.bottom = bottomRight.y;
            ClipCursor(&rect);
        }
        else
        {
            // Show cursor and release clip
            SetCursorVisible(true);
            ClipCursor(nullptr);
            ReleaseCapture();
        }
    }

    // ========================================================================
    // Factory
    // ========================================================================

    UniquePtr<IWindow> IWindow::Create(const WindowConfig& config)
    {
        return MakeUnique<WindowsWindow>(config);
    }

} // namespace Forge::Platform

#endif // FORGE_PLATFORM_WINDOWS
