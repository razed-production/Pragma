#pragma once

#include "Pragma/Platform/InputState.h"
#include "Pragma/RHI/Types.h"

#include <cstddef>
#include <functional>
#include <string_view>

namespace Pragma::Platform
{
class Window
{
public:
    Window(std::wstring_view title, std::uint32_t width, std::uint32_t height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] bool PumpMessages();
    void EndFrame() noexcept;
    [[nodiscard]] Pragma::RHI::NativeWindow GetNativeWindow() const noexcept;
    [[nodiscard]] Pragma::RHI::Extent2D GetExtent() const noexcept;
    [[nodiscard]] const InputState& GetInputState() const noexcept;
    [[nodiscard]] bool IsFullscreen() const noexcept;
    void SetMessageHandler(std::function<bool(void*, unsigned int, std::size_t, std::ptrdiff_t)> handler);

private:
    void ToggleFullscreen();
    void ResetPerFrameInput() noexcept;
    static void RegisterWindowClass();
    static long long __stdcall WindowProc(void* hwnd, unsigned int message, std::size_t wParam, std::ptrdiff_t lParam);
    long long HandleMessage(unsigned int message, std::size_t wParam, std::ptrdiff_t lParam);

private:
    void* m_windowHandle = nullptr;
    Pragma::RHI::Extent2D m_extent;
    InputState m_inputState;
    std::function<bool(void*, unsigned int, std::size_t, std::ptrdiff_t)> m_messageHandler;
    bool m_shouldClose = false;
    bool m_isFullscreen = false;
    bool m_hasMousePosition = false;
    std::int32_t m_lastMouseX = 0;
    std::int32_t m_lastMouseY = 0;
    long m_windowedStyle = 0;
    struct WindowRect
    {
        long Left = 0;
        long Top = 0;
        long Right = 0;
        long Bottom = 0;
    } m_windowedRect;
};
}
