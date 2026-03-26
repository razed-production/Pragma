#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Pragma::Renderer
{
inline std::filesystem::path GetRendererExecutableDirectory()
{
    std::array<wchar_t, 4096> modulePath{};
    const DWORD length = ::GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (length == 0 || length >= modulePath.size())
    {
        return {};
    }

    return std::filesystem::path(std::wstring(modulePath.data(), length)).parent_path();
}

inline std::string ReadRendererTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open renderer text file: " + path.string());
    }

    std::ostringstream stream;
    stream << input.rdbuf();
    return stream.str();
}

inline std::string LoadRendererShaderSource(const std::filesystem::path& relativePath)
{
    std::vector<std::filesystem::path> candidates;
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path exeDir = GetRendererExecutableDirectory();

    candidates.push_back(cwd / "assets" / "shaders" / relativePath);
    if (!exeDir.empty())
    {
        candidates.push_back(exeDir / "assets" / "shaders" / relativePath);
        candidates.push_back(exeDir.parent_path() / "assets" / "shaders" / relativePath);
        candidates.push_back(exeDir.parent_path().parent_path() / "assets" / "shaders" / relativePath);
    }

    for (const auto& candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            return ReadRendererTextFile(candidate);
        }
    }

    std::string searchedPaths;
    for (const auto& candidate : candidates)
    {
        if (!searchedPaths.empty())
        {
            searchedPaths += "; ";
        }
        searchedPaths += candidate.string();
    }

    throw std::runtime_error("Failed to resolve renderer shader source '" + relativePath.string() + "'. Searched: " + searchedPaths);
}
}
