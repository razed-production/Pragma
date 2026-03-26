#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Pragma::Core
{
enum class LogLevel
{
    Info,
    Warning,
    Error
};

enum class LogCategory
{
    General,
    Assets,
    Renderer,
    RHI,
    Scene,
    DebugUI,
    Platform
};

struct LogEntry
{
    std::string Timestamp;
    LogCategory Category = LogCategory::General;
    LogLevel Level = LogLevel::Info;
    std::string Message;
};

inline const char* ToString(const LogLevel level) noexcept
{
    switch (level)
    {
    case LogLevel::Info:
        return "Info";
    case LogLevel::Warning:
        return "Warn";
    case LogLevel::Error:
        return "Error";
    default:
        return "Log";
    }
}

inline const char* ToString(const LogCategory category) noexcept
{
    switch (category)
    {
    case LogCategory::Assets:
        return "Assets";
    case LogCategory::Renderer:
        return "Renderer";
    case LogCategory::RHI:
        return "RHI";
    case LogCategory::Scene:
        return "Scene";
    case LogCategory::DebugUI:
        return "DebugUI";
    case LogCategory::Platform:
        return "Platform";
    case LogCategory::General:
    default:
        return "General";
    }
}

inline std::vector<LogEntry>& GetMutableLogEntries() noexcept
{
    static std::vector<LogEntry> entries;
    return entries;
}

inline std::mutex& GetLogMutex() noexcept
{
    static std::mutex mutex;
    return mutex;
}

inline constexpr std::size_t kMaxLogEntries = 1024;

inline void Log(const LogCategory category, const LogLevel level, const std::string_view message)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
    localtime_s(&localTime, &timestamp);

    std::ostringstream timestampStream;
    timestampStream << std::put_time(&localTime, "%H:%M:%S");
    const std::string timestampText = timestampStream.str();

    {
        std::scoped_lock lock(GetLogMutex());
        std::vector<LogEntry>& entries = GetMutableLogEntries();
        entries.push_back({ timestampText, category, level, std::string(message) });
        if (entries.size() > kMaxLogEntries)
        {
            entries.erase(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(entries.size() - kMaxLogEntries));
        }
    }

    std::ostream& stream = level == LogLevel::Error ? std::cerr : std::cout;
    stream << '[' << timestampText << "] "
           << '[' << ToString(level) << "] "
           << '[' << ToString(category) << "] "
           << message << '\n'
           << std::flush;
}

inline void Log(const LogLevel level, const std::string_view message)
{
    Log(LogCategory::General, level, message);
}

inline std::vector<LogEntry> GetLogEntriesSnapshot()
{
    std::scoped_lock lock(GetLogMutex());
    return GetMutableLogEntries();
}
}
