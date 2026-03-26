#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Pragma/Scripting/DotNetHostFxr.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace Pragma::Scripting
{
namespace
{
using hostfxr_handle = void*;

struct hostfxr_initialize_parameters;

using hostfxr_initialize_for_runtime_config_fn = int(__cdecl*)(const wchar_t*, const hostfxr_initialize_parameters*, hostfxr_handle*);
using hostfxr_get_runtime_delegate_fn = int(__cdecl*)(hostfxr_handle, int, void**);
using hostfxr_close_fn = int(__cdecl*)(hostfxr_handle);
using load_assembly_and_get_function_pointer_fn = int(__cdecl*)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*, void*, void**);

enum hostfxr_delegate_type
{
    hdt_com_activation = 0,
    hdt_load_in_memory_assembly = 1,
    hdt_winrt_activation = 2,
    hdt_com_register = 3,
    hdt_com_unregister = 4,
    hdt_load_assembly_and_get_function_pointer = 5,
    hdt_get_function_pointer = 6
};

const wchar_t* GetUnmanagedCallersOnlyMethod() noexcept
{
    return reinterpret_cast<const wchar_t*>(static_cast<intptr_t>(-1));
}

std::filesystem::path GetDotNetRootCandidate()
{
    wchar_t* envValue = nullptr;
    std::size_t envLength = 0;
    if (_wdupenv_s(&envValue, &envLength, L"DOTNET_ROOT") == 0 && envValue != nullptr && envLength > 0)
    {
        std::filesystem::path root(envValue);
        std::free(envValue);
        return root;
    }

    std::free(envValue);
    return std::filesystem::path(L"C:\\Program Files\\dotnet");
}

std::filesystem::path FindLatestHostFxrLibrary()
{
    const std::filesystem::path hostFxrRoot = GetDotNetRootCandidate() / "host" / "fxr";
    if (!std::filesystem::exists(hostFxrRoot))
    {
        return {};
    }

    std::vector<std::filesystem::path> versionDirectories;
    for (const auto& entry : std::filesystem::directory_iterator(hostFxrRoot))
    {
        if (entry.is_directory())
        {
            versionDirectories.push_back(entry.path());
        }
    }

    if (versionDirectories.empty())
    {
        return {};
    }

    std::sort(versionDirectories.begin(), versionDirectories.end());
    std::reverse(versionDirectories.begin(), versionDirectories.end());

    for (const std::filesystem::path& versionDirectory : versionDirectories)
    {
        const std::filesystem::path candidate = versionDirectory / "hostfxr.dll";
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    return {};
}
}

DotNetHostFxr::~DotNetHostFxr()
{
    Unload();
}

bool DotNetHostFxr::Load()
{
    Unload();

    const std::filesystem::path libraryPath = FindLatestHostFxrLibrary();
    if (libraryPath.empty())
    {
        m_statusText = "hostfxr.dll not found in .NET installation.";
        return false;
    }

    HMODULE module = ::LoadLibraryW(libraryPath.c_str());
    if (module == nullptr)
    {
        m_statusText = "Failed to load hostfxr.dll.";
        return false;
    }

    const auto initializeForRuntimeConfig =
        reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(::GetProcAddress(module, "hostfxr_initialize_for_runtime_config"));
    const auto getRuntimeDelegate =
        reinterpret_cast<hostfxr_get_runtime_delegate_fn>(::GetProcAddress(module, "hostfxr_get_runtime_delegate"));
    const auto close =
        reinterpret_cast<hostfxr_close_fn>(::GetProcAddress(module, "hostfxr_close"));

    if (initializeForRuntimeConfig == nullptr || getRuntimeDelegate == nullptr || close == nullptr)
    {
        ::FreeLibrary(module);
        m_statusText = "hostfxr.dll loaded, but required exports are missing.";
        return false;
    }

    m_module = module;
    m_libraryPath = libraryPath;
    m_statusText = "hostfxr bootstrap ready.";
    return true;
}

bool DotNetHostFxr::ProbeRuntimeConfig(const std::filesystem::path& runtimeConfigPath, std::string& status) const
{
    if (!IsLoaded())
    {
        status = "hostfxr is not loaded.";
        return false;
    }

    if (!std::filesystem::exists(runtimeConfigPath))
    {
        status = "runtimeconfig.json is missing.";
        return false;
    }

    const auto initializeForRuntimeConfig =
        reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_initialize_for_runtime_config"));
    const auto close =
        reinterpret_cast<hostfxr_close_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_close"));
    if (initializeForRuntimeConfig == nullptr || close == nullptr)
    {
        status = "hostfxr exports are unavailable.";
        return false;
    }

    hostfxr_handle context = nullptr;
    const int result = initializeForRuntimeConfig(runtimeConfigPath.c_str(), nullptr, &context);
    if (result != 0 || context == nullptr)
    {
        status = "hostfxr_initialize_for_runtime_config failed with code " + std::to_string(result) + ".";
        return false;
    }

    close(context);
    status = "runtimeconfig bootstrap succeeded.";
    return true;
}

bool DotNetHostFxr::LoadUnmanagedFunctionPointer(
    const std::filesystem::path& runtimeConfigPath,
    const std::filesystem::path& assemblyPath,
    const wchar_t* typeName,
    const wchar_t* methodName,
    void** functionPointer,
    std::string& status) const
{
    if (functionPointer == nullptr)
    {
        status = "function pointer output is null.";
        return false;
    }

    *functionPointer = nullptr;

    if (!IsLoaded())
    {
        status = "hostfxr is not loaded.";
        return false;
    }

    if (!std::filesystem::exists(runtimeConfigPath))
    {
        status = "runtimeconfig.json is missing.";
        return false;
    }

    if (!std::filesystem::exists(assemblyPath))
    {
        status = "managed assembly is missing.";
        return false;
    }

    const auto initializeForRuntimeConfig =
        reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_initialize_for_runtime_config"));
    const auto getRuntimeDelegate =
        reinterpret_cast<hostfxr_get_runtime_delegate_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_get_runtime_delegate"));
    const auto close =
        reinterpret_cast<hostfxr_close_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_close"));
    if (initializeForRuntimeConfig == nullptr || getRuntimeDelegate == nullptr || close == nullptr)
    {
        status = "hostfxr exports are unavailable.";
        return false;
    }

    hostfxr_handle context = nullptr;
    const int initResult = initializeForRuntimeConfig(runtimeConfigPath.c_str(), nullptr, &context);
    if (initResult != 0 || context == nullptr)
    {
        status = "hostfxr_initialize_for_runtime_config failed with code " + std::to_string(initResult) + ".";
        return false;
    }

    load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer = nullptr;
    const int delegateResult = getRuntimeDelegate(
        context,
        hdt_load_assembly_and_get_function_pointer,
        reinterpret_cast<void**>(&loadAssemblyAndGetFunctionPointer));
    if (delegateResult != 0 || loadAssemblyAndGetFunctionPointer == nullptr)
    {
        close(context);
        status = "hostfxr_get_runtime_delegate failed with code " + std::to_string(delegateResult) + ".";
        return false;
    }

    const int loadResult = loadAssemblyAndGetFunctionPointer(
        assemblyPath.c_str(),
        typeName,
        methodName,
        GetUnmanagedCallersOnlyMethod(),
        nullptr,
        functionPointer);

    close(context);

    if (loadResult != 0 || *functionPointer == nullptr)
    {
        status = "load_assembly_and_get_function_pointer failed with code " + std::to_string(loadResult) + ".";
        return false;
    }

    status = "managed entry point resolved.";
    return true;
}

bool DotNetHostFxr::LoadManagedScriptApi(
    const std::filesystem::path& runtimeConfigPath,
    const std::filesystem::path& assemblyPath,
    const wchar_t* typeName,
    void** createInstanceFunctionPointer,
    void** startInstanceFunctionPointer,
    void** updateInstanceFunctionPointer,
    void** destroyInstanceFunctionPointer,
    std::string& status) const
{
    if (createInstanceFunctionPointer == nullptr ||
        startInstanceFunctionPointer == nullptr ||
        updateInstanceFunctionPointer == nullptr ||
        destroyInstanceFunctionPointer == nullptr)
    {
        status = "managed script api outputs are null.";
        return false;
    }

    *createInstanceFunctionPointer = nullptr;
    *startInstanceFunctionPointer = nullptr;
    *updateInstanceFunctionPointer = nullptr;
    *destroyInstanceFunctionPointer = nullptr;

    if (!IsLoaded())
    {
        status = "hostfxr is not loaded.";
        return false;
    }

    if (!std::filesystem::exists(runtimeConfigPath))
    {
        status = "runtimeconfig.json is missing.";
        return false;
    }

    if (!std::filesystem::exists(assemblyPath))
    {
        status = "managed assembly is missing.";
        return false;
    }

    const auto initializeForRuntimeConfig =
        reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_initialize_for_runtime_config"));
    const auto getRuntimeDelegate =
        reinterpret_cast<hostfxr_get_runtime_delegate_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_get_runtime_delegate"));
    const auto close =
        reinterpret_cast<hostfxr_close_fn>(::GetProcAddress(static_cast<HMODULE>(m_module), "hostfxr_close"));
    if (initializeForRuntimeConfig == nullptr || getRuntimeDelegate == nullptr || close == nullptr)
    {
        status = "hostfxr exports are unavailable.";
        return false;
    }

    hostfxr_handle context = nullptr;
    const int initResult = initializeForRuntimeConfig(runtimeConfigPath.c_str(), nullptr, &context);
    if (initResult != 0 || context == nullptr)
    {
        status = "hostfxr_initialize_for_runtime_config failed with code " + std::to_string(initResult) + ".";
        return false;
    }

    load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer = nullptr;
    const int delegateResult = getRuntimeDelegate(
        context,
        hdt_load_assembly_and_get_function_pointer,
        reinterpret_cast<void**>(&loadAssemblyAndGetFunctionPointer));
    if (delegateResult != 0 || loadAssemblyAndGetFunctionPointer == nullptr)
    {
        close(context);
        status = "hostfxr_get_runtime_delegate failed with code " + std::to_string(delegateResult) + ".";
        return false;
    }

    const auto loadMethod = [&](const wchar_t* methodName, void** destination) -> int
    {
        return loadAssemblyAndGetFunctionPointer(
            assemblyPath.c_str(),
            typeName,
            methodName,
            GetUnmanagedCallersOnlyMethod(),
            nullptr,
            destination);
    };

    const int createResult = loadMethod(L"CreateManagedScriptInstance", createInstanceFunctionPointer);
    const int startResult = loadMethod(L"StartManagedScriptInstance", startInstanceFunctionPointer);
    const int updateResult = loadMethod(L"UpdateManagedScriptInstance", updateInstanceFunctionPointer);
    const int destroyResult = loadMethod(L"DestroyManagedScriptInstance", destroyInstanceFunctionPointer);
    close(context);

    if (createResult != 0 || *createInstanceFunctionPointer == nullptr)
    {
        status = "failed to resolve CreateManagedScriptInstance with code " + std::to_string(createResult) + ".";
        return false;
    }
    if (startResult != 0 || *startInstanceFunctionPointer == nullptr)
    {
        status = "failed to resolve StartManagedScriptInstance with code " + std::to_string(startResult) + ".";
        return false;
    }
    if (updateResult != 0 || *updateInstanceFunctionPointer == nullptr)
    {
        status = "failed to resolve UpdateManagedScriptInstance with code " + std::to_string(updateResult) + ".";
        return false;
    }
    if (destroyResult != 0 || *destroyInstanceFunctionPointer == nullptr)
    {
        status = "failed to resolve DestroyManagedScriptInstance with code " + std::to_string(destroyResult) + ".";
        return false;
    }

    status = "managed script api resolved.";
    return true;
}

void DotNetHostFxr::Unload() noexcept
{
    if (m_module != nullptr)
    {
        ::FreeLibrary(static_cast<HMODULE>(m_module));
        m_module = nullptr;
    }

    m_libraryPath.clear();
}

bool DotNetHostFxr::IsLoaded() const noexcept
{
    return m_module != nullptr;
}

const std::filesystem::path& DotNetHostFxr::GetLibraryPath() const noexcept
{
    return m_libraryPath;
}

const std::string& DotNetHostFxr::GetStatusText() const noexcept
{
    return m_statusText;
}
}
