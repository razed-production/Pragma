#pragma once

#include "Pragma/Assets/AssetId.h"
#include "Pragma/Core/EngineInput.h"
#include "Pragma/Scripting/DotNetHostFxr.h"
#include "Pragma/Renderer/Entity.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Pragma::Assets
{
class AssetManager;
}

namespace Pragma::Renderer
{
class Scene;
}

namespace Pragma::Scripting
{
struct ManagedScriptProject
{
    Pragma::Assets::AssetId Asset;
    std::string Name;
    std::filesystem::path Path;
    bool Exists = false;
    std::filesystem::path RuntimeConfigPath;
    bool HasRuntimeConfig = false;
    bool RuntimeReady = false;
    std::string RuntimeStatus;
    std::filesystem::path AssemblyPath;
    bool HasAssembly = false;
    bool EntryPointReady = false;
    int EntryPointProbeValue = 0;
    std::string EntryPointStatus;
    bool BindingReady = false;
    int BindingProbeValue = 0;
    std::string BindingStatus;
    bool ScriptApiReady = false;
    std::string ScriptApiStatus;
};

struct ManagedScriptTimeSnapshot
{
    float DeltaSeconds = 0.0f;
    float ElapsedSeconds = 0.0f;
    std::uint64_t FrameIndex = 0;
    const Pragma::Core::EngineInput* Input = nullptr;
};

struct ManagedScriptTypeMetadata
{
    Pragma::Assets::AssetId ProjectAsset;
    std::string TypeName;
    std::string DisplayName;
    std::string Description;
};

class ManagedScriptHost
{
public:
    void Initialize(const Pragma::Assets::AssetManager& assets);
    void RunBindingProbe(Pragma::Renderer::Scene& scene);
    [[nodiscard]] bool CreateScriptInstance(
        const Pragma::Assets::AssetId& projectAssetId,
        std::string_view typeName,
        Pragma::Renderer::EntityId entityId,
        int& instanceHandle,
        std::string& status) const;
    [[nodiscard]] bool StartScriptInstance(
        const Pragma::Assets::AssetId& projectAssetId,
        int instanceHandle,
        Pragma::Renderer::Scene& scene,
        const ManagedScriptTimeSnapshot& time,
        std::string& status) const;
    [[nodiscard]] bool UpdateScriptInstance(
        const Pragma::Assets::AssetId& projectAssetId,
        int instanceHandle,
        Pragma::Renderer::Scene& scene,
        const ManagedScriptTimeSnapshot& time,
        std::string& status) const;
    [[nodiscard]] bool DestroyScriptInstance(
        const Pragma::Assets::AssetId& projectAssetId,
        int instanceHandle,
        Pragma::Renderer::Scene& scene,
        const ManagedScriptTimeSnapshot& time,
        std::string& status) const;
    void Shutdown() noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] std::size_t GetProjectCount() const noexcept;
    [[nodiscard]] std::size_t GetRuntimeReadyProjectCount() const noexcept;
    [[nodiscard]] std::size_t GetEntryPointReadyProjectCount() const noexcept;
    [[nodiscard]] std::size_t GetBindingReadyProjectCount() const noexcept;
    [[nodiscard]] const std::vector<ManagedScriptProject>& GetProjects() const noexcept;
    [[nodiscard]] const std::vector<ManagedScriptTypeMetadata>& GetAvailableScriptTypes() const noexcept;
    [[nodiscard]] const ManagedScriptProject* FindProject(std::string_view assetId) const noexcept;
    [[nodiscard]] bool IsHostFxrAvailable() const noexcept;
    [[nodiscard]] const std::filesystem::path& GetHostFxrPath() const noexcept;
    [[nodiscard]] const std::string& GetHostFxrStatus() const noexcept;

private:
    struct ManagedScriptApi
    {
        Pragma::Assets::AssetId ProjectAsset;
        void* CreateInstance = nullptr;
        void* StartInstance = nullptr;
        void* UpdateInstance = nullptr;
        void* DestroyInstance = nullptr;
    };

    std::vector<ManagedScriptProject> m_projects;
    std::vector<ManagedScriptApi> m_scriptApis;
    std::vector<ManagedScriptTypeMetadata> m_scriptTypes;
    DotNetHostFxr m_hostFxr;
    bool m_initialized = false;
};
}
