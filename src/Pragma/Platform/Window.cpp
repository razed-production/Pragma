#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Pragma/Platform/Window.h"

#include "Pragma/Core/Log.h"

#include <stdexcept>

#include <windows.h>
#include <windowsx.h>

namespace
{
constexpr wchar_t kWindowClassName[] = L"PragmaWindowClass";
}

namespace Pragma::Platform
{
Window::Window(const std::wstring_view title, const std::uint32_t width, const std::uint32_t height)
    : m_extent{ width, height }
{
    RegisterWindowClass();

    RECT rect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        title.data(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (hwnd == nullptr)
    {
        throw std::runtime_error("Failed to create Win32 window.");
    }

    m_windowHandle = hwnd;
    m_windowedStyle = static_cast<long>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    RECT windowRect{};
    GetWindowRect(hwnd, &windowRect);
    m_windowedRect = { windowRect.left, windowRect.top, windowRect.right, windowRect.bottom };
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

Window::~Window()
{
    if (m_windowHandle != nullptr)
    {
        DestroyWindow(static_cast<HWND>(m_windowHandle));
    }
}

bool Window::PumpMessages()
{
    MSG message{};

    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            m_shouldClose = true;
            break;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return !m_shouldClose;
}

void Window::EndFrame() noexcept
{
    ResetPerFrameInput();
}

Pragma::RHI::NativeWindow Window::GetNativeWindow() const noexcept
{
    return { m_windowHandle };
}

Pragma::RHI::Extent2D Window::GetExtent() const noexcept
{
    return m_extent;
}

const InputState& Window::GetInputState() const noexcept
{
    return m_inputState;
}

bool Window::IsFullscreen() const noexcept
{
    return m_isFullscreen;
}

void Window::SetMessageHandler(std::function<bool(void*, unsigned int, std::size_t, std::ptrdiff_t)> handler)
{
    m_messageHandler = std::move(handler);
}

void Window::ResetPerFrameInput() noexcept
{
    m_inputState.MouseDeltaX = 0;
    m_inputState.MouseDeltaY = 0;
}

void Window::ToggleFullscreen()
{
    HWND hwnd = static_cast<HWND>(m_windowHandle);
    if (hwnd == nullptr)
    {
        return;
    }

    Pragma::Core::Log(
        Pragma::Core::LogCategory::Platform,
        Pragma::Core::LogLevel::Info,
        std::string("ToggleFullscreen requested. Entering fullscreen=") + (m_isFullscreen ? "false" : "true"));

    if (!m_isFullscreen)
    {
        RECT currentRect{};
        GetWindowRect(hwnd, &currentRect);
        m_windowedRect = { currentRect.left, currentRect.top, currentRect.right, currentRect.bottom };
        m_windowedStyle = static_cast<long>(GetWindowLongPtrW(hwnd, GWL_STYLE));

        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoW(monitor, &monitorInfo))
        {
            return;
        }

        SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(m_windowedStyle & ~WS_OVERLAPPEDWINDOW));
        SetWindowPos(
            hwnd,
            HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
        m_isFullscreen = true;
        Pragma::Core::Log(Pragma::Core::LogCategory::Platform, Pragma::Core::LogLevel::Info, "Entered borderless fullscreen.");
    }
    else
    {
        SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(m_windowedStyle));
        SetWindowPos(
            hwnd,
            nullptr,
            m_windowedRect.Left,
            m_windowedRect.Top,
            m_windowedRect.Right - m_windowedRect.Left,
            m_windowedRect.Bottom - m_windowedRect.Top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOZORDER);
        ShowWindow(hwnd, SW_SHOW);
        m_isFullscreen = false;
        Pragma::Core::Log(Pragma::Core::LogCategory::Platform, Pragma::Core::LogLevel::Info, "Returned to windowed mode.");
    }
}

void Window::RegisterWindowClass()
{
    static bool isRegistered = false;

    if (isRegistered)
    {
        return;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = reinterpret_cast<WNDPROC>(&Window::WindowProc);
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;

    if (RegisterClassExW(&windowClass) == 0)
    {
        throw std::runtime_error("Failed to register Win32 window class.");
    }

    isRegistered = true;
}

long long __stdcall Window::WindowProc(void* hwnd, const unsigned int message, const std::size_t wParam, const std::ptrdiff_t lParam)
{
    HWND nativeHwnd = static_cast<HWND>(hwnd);

    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        Window* window = static_cast<Window*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(nativeHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return TRUE;
    }

    Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(nativeHwnd, GWLP_USERDATA));
    if (window != nullptr)
    {
        return window->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(nativeHwnd, message, static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
}

long long Window::HandleMessage(const unsigned int message, const std::size_t wParam, const std::ptrdiff_t lParam)
{
    if (m_messageHandler && m_messageHandler(m_windowHandle, message, wParam, lParam))
    {
        return 0;
    }

    switch (message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam == VK_F11)
        {
            ToggleFullscreen();
            return 0;
        }
        if (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000) != 0)
        {
            ToggleFullscreen();
            return 0;
        }
        switch (wParam)
        {
        case 'W': m_inputState.MoveForward = true; break;
        case 'S': m_inputState.MoveBackward = true; break;
        case 'A': m_inputState.MoveLeft = true; break;
        case 'D': m_inputState.MoveRight = true; break;
        case 'E': m_inputState.MoveUp = true; break;
        case 'Q': m_inputState.MoveDown = true; break;
        case VK_LEFT: m_inputState.LookLeft = true; break;
        case VK_RIGHT: m_inputState.LookRight = true; break;
        case VK_UP: m_inputState.LookUp = true; break;
        case VK_DOWN: m_inputState.LookDown = true; break;
        case VK_SHIFT: m_inputState.FastMove = true; break;
        case VK_ESCAPE:
            m_shouldClose = true;
            DestroyWindow(static_cast<HWND>(m_windowHandle));
            m_windowHandle = nullptr;
            break;
        default:
            break;
        }
        return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam == VK_F11 || (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000) != 0))
        {
            return 0;
        }
        switch (wParam)
        {
        case 'W': m_inputState.MoveForward = false; break;
        case 'S': m_inputState.MoveBackward = false; break;
        case 'A': m_inputState.MoveLeft = false; break;
        case 'D': m_inputState.MoveRight = false; break;
        case 'E': m_inputState.MoveUp = false; break;
        case 'Q': m_inputState.MoveDown = false; break;
        case VK_LEFT: m_inputState.LookLeft = false; break;
        case VK_RIGHT: m_inputState.LookRight = false; break;
        case VK_UP: m_inputState.LookUp = false; break;
        case VK_DOWN: m_inputState.LookDown = false; break;
        case VK_SHIFT: m_inputState.FastMove = false; break;
        default:
            break;
        }
        return 0;

    case WM_SIZE:
        m_extent.Width = LOWORD(lParam);
        m_extent.Height = HIWORD(lParam);
        Pragma::Core::Log(
            Pragma::Core::LogCategory::Platform,
            Pragma::Core::LogLevel::Info,
            "WM_SIZE -> " + std::to_string(m_extent.Width) + "x" + std::to_string(m_extent.Height));
        return 0;

    case WM_RBUTTONDOWN:
        m_inputState.RightMouseButtonDown = true;
        SetCapture(static_cast<HWND>(m_windowHandle));
        m_lastMouseX = GET_X_LPARAM(lParam);
        m_lastMouseY = GET_Y_LPARAM(lParam);
        m_hasMousePosition = true;
        return 0;

    case WM_RBUTTONUP:
        m_inputState.RightMouseButtonDown = false;
        ReleaseCapture();
        m_hasMousePosition = false;
        return 0;

    case WM_MOUSEMOVE:
    {
        const std::int32_t mouseX = GET_X_LPARAM(lParam);
        const std::int32_t mouseY = GET_Y_LPARAM(lParam);

        if (m_inputState.RightMouseButtonDown && m_hasMousePosition)
        {
            m_inputState.MouseDeltaX += mouseX - m_lastMouseX;
            m_inputState.MouseDeltaY += mouseY - m_lastMouseY;
        }

        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        m_hasMousePosition = true;
        return 0;
    }

    case WM_CLOSE:
        m_shouldClose = true;
        DestroyWindow(static_cast<HWND>(m_windowHandle));
        m_windowHandle = nullptr;
        return 0;

    case WM_DESTROY:
        m_shouldClose = true;
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(m_windowHandle != nullptr ? static_cast<HWND>(m_windowHandle) : nullptr, message, static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
    }
}
}
