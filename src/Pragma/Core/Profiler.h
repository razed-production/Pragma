#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Pragma::Core
{
struct ProfileEvent
{
    std::string Name;
    std::uint32_t Depth = 0;
    double DurationMilliseconds = 0.0;
};

struct FrameProfile
{
    std::uint64_t FrameIndex = 0;
    double TotalMilliseconds = 0.0;
    std::vector<ProfileEvent> Events;
};

struct GpuProfileEvent
{
    std::string Name;
    double DurationMilliseconds = 0.0;
};

struct GpuFrameProfile
{
    std::uint64_t FrameIndex = 0;
    double TotalMilliseconds = 0.0;
    bool IsValid = false;
    std::vector<GpuProfileEvent> Events;
};

inline std::mutex& GetProfilerMutex() noexcept
{
    static std::mutex mutex;
    return mutex;
}

inline FrameProfile& GetCurrentFrameProfile() noexcept
{
    static FrameProfile profile;
    return profile;
}

inline FrameProfile& GetLastCompletedFrameProfile() noexcept
{
    static FrameProfile profile;
    return profile;
}

inline GpuFrameProfile& GetLastCompletedGpuFrameProfile() noexcept
{
    static GpuFrameProfile profile;
    return profile;
}

inline double& GetLastCompletedFrameMilliseconds() noexcept
{
    static double milliseconds = 0.0;
    return milliseconds;
}

inline double& GetLastCompletedGpuFrameMilliseconds() noexcept
{
    static double milliseconds = 0.0;
    return milliseconds;
}

inline double& GetMutablePeakFrameMilliseconds() noexcept
{
    static double milliseconds = 0.0;
    return milliseconds;
}

inline std::uint64_t& GetMutablePeakFrameIndex() noexcept
{
    static std::uint64_t frameIndex = 0;
    return frameIndex;
}

inline double& GetMutablePeakGpuFrameMilliseconds() noexcept
{
    static double milliseconds = 0.0;
    return milliseconds;
}

inline std::uint64_t& GetMutablePeakGpuFrameIndex() noexcept
{
    static std::uint64_t frameIndex = 0;
    return frameIndex;
}

inline std::uint64_t& GetMutableCurrentFrameIndex() noexcept
{
    static std::uint64_t frameIndex = 0;
    return frameIndex;
}

inline thread_local std::vector<std::size_t> g_profileScopeStack;

inline void BeginProfileFrame(const std::uint64_t frameIndex)
{
    std::scoped_lock lock(GetProfilerMutex());
    GetMutableCurrentFrameIndex() = frameIndex;
    FrameProfile& currentFrame = GetCurrentFrameProfile();
    currentFrame.FrameIndex = frameIndex;
    currentFrame.TotalMilliseconds = 0.0;
    currentFrame.Events.clear();
    g_profileScopeStack.clear();
}

inline void EndProfileFrame(const double totalMilliseconds)
{
    std::scoped_lock lock(GetProfilerMutex());
    FrameProfile& currentFrame = GetCurrentFrameProfile();
    currentFrame.TotalMilliseconds = totalMilliseconds;
    GetLastCompletedFrameProfile() = currentFrame;
    GetLastCompletedFrameMilliseconds() = totalMilliseconds;

    if (totalMilliseconds > GetMutablePeakFrameMilliseconds())
    {
        GetMutablePeakFrameMilliseconds() = totalMilliseconds;
        GetMutablePeakFrameIndex() = currentFrame.FrameIndex;
    }
}

inline FrameProfile GetLastFrameProfileSnapshot()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetLastCompletedFrameProfile();
}

inline double GetLastFrameMilliseconds()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetLastCompletedFrameMilliseconds();
}

inline double GetPeakFrameMilliseconds()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetMutablePeakFrameMilliseconds();
}

inline std::uint64_t GetPeakFrameIndex()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetMutablePeakFrameIndex();
}

inline std::uint64_t GetCurrentProfileFrameIndex()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetMutableCurrentFrameIndex();
}

inline void SubmitGpuFrameProfile(const GpuFrameProfile& profile)
{
    std::scoped_lock lock(GetProfilerMutex());
    GetLastCompletedGpuFrameProfile() = profile;
    GetLastCompletedGpuFrameMilliseconds() = profile.TotalMilliseconds;

    if (profile.TotalMilliseconds > GetMutablePeakGpuFrameMilliseconds())
    {
        GetMutablePeakGpuFrameMilliseconds() = profile.TotalMilliseconds;
        GetMutablePeakGpuFrameIndex() = profile.FrameIndex;
    }
}

inline GpuFrameProfile GetLastGpuFrameProfileSnapshot()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetLastCompletedGpuFrameProfile();
}

inline double GetLastGpuFrameMilliseconds()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetLastCompletedGpuFrameMilliseconds();
}

inline double GetPeakGpuFrameMilliseconds()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetMutablePeakGpuFrameMilliseconds();
}

inline std::uint64_t GetPeakGpuFrameIndex()
{
    std::scoped_lock lock(GetProfilerMutex());
    return GetMutablePeakGpuFrameIndex();
}

class ProfileScope
{
public:
    explicit ProfileScope(const std::string_view name)
        : m_start(std::chrono::steady_clock::now())
    {
        std::scoped_lock lock(GetProfilerMutex());
        FrameProfile& currentFrame = GetCurrentFrameProfile();
        currentFrame.Events.push_back({ std::string(name), static_cast<std::uint32_t>(g_profileScopeStack.size()), 0.0 });
        m_eventIndex = currentFrame.Events.size() - 1;
        g_profileScopeStack.push_back(m_eventIndex);
    }

    ~ProfileScope()
    {
        const auto end = std::chrono::steady_clock::now();
        const double durationMilliseconds = std::chrono::duration<double, std::milli>(end - m_start).count();

        std::scoped_lock lock(GetProfilerMutex());
        FrameProfile& currentFrame = GetCurrentFrameProfile();
        if (m_eventIndex < currentFrame.Events.size())
        {
            currentFrame.Events[m_eventIndex].DurationMilliseconds = durationMilliseconds;
        }

        if (!g_profileScopeStack.empty())
        {
            g_profileScopeStack.pop_back();
        }
    }

private:
    std::chrono::steady_clock::time_point m_start;
    std::size_t m_eventIndex = 0;
};
}

#define PRAGMA_PROFILE_SCOPE_CONCAT_IMPL(a, b) a##b
#define PRAGMA_PROFILE_SCOPE_CONCAT(a, b) PRAGMA_PROFILE_SCOPE_CONCAT_IMPL(a, b)
#define PRAGMA_PROFILE_SCOPE(nameLiteral) ::Pragma::Core::ProfileScope PRAGMA_PROFILE_SCOPE_CONCAT(pragmaProfileScope, __LINE__)(nameLiteral)
