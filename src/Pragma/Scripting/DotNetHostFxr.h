#pragma once

#include <filesystem>
#include <string>

namespace Pragma::Scripting
{
class DotNetHostFxr
{
public:
    DotNetHostFxr() = default;
    ~DotNetHostFxr();

    [[nodiscard]] bool Load();
    [[nodiscard]] bool ProbeRuntimeConfig(const std::filesystem::path& runtimeConfigPath, std::string& status) const;
    [[nodiscard]] bool LoadUnmanagedFunctionPointer(
        const std::filesystem::path& runtimeConfigPath,
        const std::filesystem::path& assemblyPath,
        const wchar_t* typeName,
        const wchar_t* methodName,
        void** functionPointer,
        std::string& status) const;
    [[nodiscard]] bool LoadManagedScriptApi(
        const std::filesystem::path& runtimeConfigPath,
        const std::filesystem::path& assemblyPath,
        const wchar_t* typeName,
        void** createInstanceFunctionPointer,
        void** startInstanceFunctionPointer,
        void** updateInstanceFunctionPointer,
        void** destroyInstanceFunctionPointer,
        std::string& status) const;
    void Unload() noexcept;

    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetLibraryPath() const noexcept;
    [[nodiscard]] const std::string& GetStatusText() const noexcept;

private:
    void* m_module = nullptr;
    std::filesystem::path m_libraryPath;
    std::string m_statusText = "Not initialized.";
};
}
