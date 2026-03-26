#include "Pragma/Scripting/ManagedScriptHost.h"

#include "Pragma/Assets/AssetManager.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Renderer/Scene.h"
#include "Pragma/Renderer/Transform.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>

namespace Pragma::Scripting
{
namespace
{
constexpr std::string_view kManagedScriptProjectPrefix = "script_project.";
const std::filesystem::path kManagedProbeTracePath = std::filesystem::current_path() / "saved" / "managed_probe_runtime.log";

struct ManagedTimeSnapshot
{
    float DeltaSeconds = 0.0f;
    float ElapsedSeconds = 0.0f;
    std::uint64_t FrameIndex = 0;
};

struct ManagedVector3
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
};

struct ManagedTransformSnapshot
{
    ManagedVector3 Position;
    ManagedVector3 RotationRadians;
    ManagedVector3 Scale{ 1.0f, 1.0f, 1.0f };
};

struct ManagedCameraSnapshot
{
    float PitchRadians = 0.0f;
    float FieldOfViewRadians = 1.0471975512f;
    float NearPlane = 0.1f;
    float FarPlane = 100.0f;
};

struct ManagedLightSnapshot
{
    ManagedVector3 Direction{ -0.4f, -0.8f, 0.3f };
    float Intensity = 1.0f;
    ManagedVector3 Color{ 1.0f, 1.0f, 1.0f };
    float Padding0 = 0.0f;
};

struct ManagedBindings
{
    void* LogCallback = nullptr;
    void* FindEntityByName = nullptr;
    void* IsEntityValid = nullptr;
    void* GetEntityName = nullptr;
    void* GetParent = nullptr;
    void* GetChildCount = nullptr;
    void* GetChildAt = nullptr;
    void* GetActiveCameraEntity = nullptr;
    void* GetEntityCount = nullptr;
    void* GetTransform = nullptr;
    void* SetTransform = nullptr;
    void* GetCamera = nullptr;
    void* SetCamera = nullptr;
    void* GetLight = nullptr;
    void* SetLight = nullptr;
};

using managed_binding_probe_fn = int(*)(ManagedTimeSnapshot, ManagedBindings, void*);
using managed_create_script_instance_fn = int(*)(const char*, std::uint64_t);
using managed_script_lifecycle_fn = int(*)(int, ManagedTimeSnapshot, ManagedBindings, void*);

void AppendManagedProbeTrace(const std::string_view line)
{
    std::error_code ec;
    std::filesystem::create_directories(kManagedProbeTracePath.parent_path(), ec);

    std::ofstream stream(kManagedProbeTracePath, std::ios::app);
    if (!stream.is_open())
    {
        return;
    }

    stream << line << '\n';
}

void __cdecl ManagedProbeLogCallback(const char* message)
{
    Pragma::Core::Log(
        Pragma::Core::LogCategory::General,
        Pragma::Core::LogLevel::Info,
        std::string("Managed callback: ") + (message != nullptr ? message : "<null>"));
    AppendManagedProbeTrace(std::string("Managed callback: ") + (message != nullptr ? message : "<null>"));
}

[[nodiscard]] ManagedVector3 ToManagedVector3(const Pragma::Math::Vector3& value) noexcept
{
    return { value.X, value.Y, value.Z };
}

[[nodiscard]] Pragma::Math::Vector3 ToVector3(const ManagedVector3& value) noexcept
{
    return { value.X, value.Y, value.Z };
}

[[nodiscard]] ManagedTransformSnapshot ToManagedTransformSnapshot(const Pragma::Renderer::Transform& transform) noexcept
{
    ManagedTransformSnapshot snapshot{};
    snapshot.Position = ToManagedVector3(transform.Position);
    snapshot.RotationRadians = ToManagedVector3(transform.RotationRadians);
    snapshot.Scale = ToManagedVector3(transform.Scale);
    return snapshot;
}

[[nodiscard]] Pragma::Renderer::Transform ToTransform(const ManagedTransformSnapshot& snapshot) noexcept
{
    Pragma::Renderer::Transform transform{};
    transform.Position = ToVector3(snapshot.Position);
    transform.RotationRadians = ToVector3(snapshot.RotationRadians);
    transform.Scale = ToVector3(snapshot.Scale);
    return transform;
}

[[nodiscard]] ManagedCameraSnapshot ToManagedCameraSnapshot(const Pragma::Renderer::CameraComponent& camera) noexcept
{
    return
    {
        camera.PitchRadians,
        camera.FieldOfViewRadians,
        camera.NearPlane,
        camera.FarPlane
    };
}

[[nodiscard]] Pragma::Renderer::CameraComponent ToCameraComponent(const ManagedCameraSnapshot& snapshot) noexcept
{
    Pragma::Renderer::CameraComponent camera{};
    camera.PitchRadians = snapshot.PitchRadians;
    camera.FieldOfViewRadians = snapshot.FieldOfViewRadians;
    camera.NearPlane = snapshot.NearPlane;
    camera.FarPlane = snapshot.FarPlane;
    return camera;
}

[[nodiscard]] ManagedLightSnapshot ToManagedLightSnapshot(const Pragma::Renderer::LightComponent& light) noexcept
{
    ManagedLightSnapshot snapshot{};
    snapshot.Direction = { light.Direction[0], light.Direction[1], light.Direction[2] };
    snapshot.Intensity = light.Intensity;
    snapshot.Color = { light.Color[0], light.Color[1], light.Color[2] };
    snapshot.Padding0 = light.Padding0;
    return snapshot;
}

[[nodiscard]] Pragma::Renderer::LightComponent ToLightComponent(const ManagedLightSnapshot& snapshot) noexcept
{
    Pragma::Renderer::LightComponent light{};
    light.Direction[0] = snapshot.Direction.X;
    light.Direction[1] = snapshot.Direction.Y;
    light.Direction[2] = snapshot.Direction.Z;
    light.Intensity = snapshot.Intensity;
    light.Color[0] = snapshot.Color.X;
    light.Color[1] = snapshot.Color.Y;
    light.Color[2] = snapshot.Color.Z;
    light.Padding0 = snapshot.Padding0;
    return light;
}

[[nodiscard]] ManagedTimeSnapshot ToManagedTimeSnapshot(const ManagedScriptTimeSnapshot& time) noexcept
{
    return
    {
        time.DeltaSeconds,
        time.ElapsedSeconds,
        time.FrameIndex
    };
}

std::uint64_t __cdecl ManagedFindEntityByNameCallback(void* sceneContext, const char* name)
{
    if (sceneContext == nullptr || name == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    return scene->FindEntityIdByName(name);
}

int __cdecl ManagedIsEntityValidCallback(void* sceneContext, const std::uint64_t entityId)
{
    if (sceneContext == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    return scene->IsEntityAlive(entityId) ? 1 : 0;
}

int __cdecl ManagedGetEntityNameCallback(void* sceneContext, const std::uint64_t entityId, char* buffer, const int bufferSize)
{
    if (sceneContext == nullptr || buffer == nullptr || bufferSize <= 0)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    const Pragma::Renderer::SceneObject* object = scene->FindObject(entityId);
    if (object == nullptr)
    {
        buffer[0] = '\0';
        return 0;
    }

    strncpy_s(buffer, static_cast<std::size_t>(bufferSize), object->Name.c_str(), _TRUNCATE);
    return 1;
}

std::uint64_t __cdecl ManagedGetParentCallback(void* sceneContext, const std::uint64_t entityId)
{
    if (sceneContext == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    return scene->GetParentId(entityId);
}

std::uint64_t __cdecl ManagedGetChildCountCallback(void* sceneContext, const std::uint64_t entityId)
{
    if (sceneContext == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    return static_cast<std::uint64_t>(scene->GetChildren(entityId).size());
}

std::uint64_t __cdecl ManagedGetChildAtCallback(void* sceneContext, const std::uint64_t entityId, const std::uint64_t childIndex)
{
    if (sceneContext == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    const std::vector<Pragma::Renderer::EntityId> children = scene->GetChildren(entityId);
    if (childIndex >= children.size())
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    return children[static_cast<std::size_t>(childIndex)];
}

std::uint64_t __cdecl ManagedGetActiveCameraEntityCallback(void* sceneContext)
{
    if (sceneContext == nullptr)
    {
        return Pragma::Renderer::InvalidEntityId;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    return scene->GetActiveCameraEntityId();
}

std::uint64_t __cdecl ManagedGetEntityCountCallback(void* sceneContext)
{
    if (sceneContext == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    return static_cast<std::uint64_t>(scene->GetObjects().size());
}

int __cdecl ManagedGetTransformCallback(void* sceneContext, const std::uint64_t entityId, ManagedTransformSnapshot* outTransform)
{
    if (sceneContext == nullptr || outTransform == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    const Pragma::Renderer::Transform* transform = scene->GetTransform(entityId);
    if (transform == nullptr)
    {
        return 0;
    }

    *outTransform = ToManagedTransformSnapshot(*transform);
    return 1;
}

int __cdecl ManagedSetTransformCallback(void* sceneContext, const std::uint64_t entityId, const ManagedTransformSnapshot* transformSnapshot)
{
    if (sceneContext == nullptr || transformSnapshot == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    Pragma::Renderer::Transform* transform = scene->GetTransform(entityId);
    if (transform == nullptr)
    {
        return 0;
    }

    *transform = ToTransform(*transformSnapshot);
    return 1;
}

int __cdecl ManagedGetCameraCallback(void* sceneContext, const std::uint64_t entityId, ManagedCameraSnapshot* outCamera)
{
    if (sceneContext == nullptr || outCamera == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    const Pragma::Renderer::CameraComponent* camera = scene->GetCamera(entityId);
    if (camera == nullptr)
    {
        return 0;
    }

    *outCamera = ToManagedCameraSnapshot(*camera);
    return 1;
}

int __cdecl ManagedSetCameraCallback(void* sceneContext, const std::uint64_t entityId, const ManagedCameraSnapshot* cameraSnapshot)
{
    if (sceneContext == nullptr || cameraSnapshot == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    Pragma::Renderer::CameraComponent* camera = scene->GetCamera(entityId);
    if (camera == nullptr)
    {
        return 0;
    }

    *camera = ToCameraComponent(*cameraSnapshot);
    return 1;
}

int __cdecl ManagedGetLightCallback(void* sceneContext, const std::uint64_t entityId, ManagedLightSnapshot* outLight)
{
    if (sceneContext == nullptr || outLight == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    const Pragma::Renderer::LightComponent* light = scene->GetLight(entityId);
    if (light == nullptr)
    {
        return 0;
    }

    *outLight = ToManagedLightSnapshot(*light);
    return 1;
}

int __cdecl ManagedSetLightCallback(void* sceneContext, const std::uint64_t entityId, const ManagedLightSnapshot* lightSnapshot)
{
    if (sceneContext == nullptr || lightSnapshot == nullptr)
    {
        return 0;
    }

    auto* scene = static_cast<Pragma::Renderer::Scene*>(sceneContext);
    Pragma::Renderer::LightComponent* light = scene->GetLight(entityId);
    if (light == nullptr)
    {
        return 0;
    }

    *light = ToLightComponent(*lightSnapshot);
    return 1;
}
}

void ManagedScriptHost::Initialize(const Pragma::Assets::AssetManager& assets)
{
    m_projects.clear();
    m_scriptApis.clear();
    m_scriptTypes.clear();
    std::error_code ec;
    std::filesystem::remove(kManagedProbeTracePath, ec);

    for (const Pragma::Assets::AssetId& assetId : assets.GetAssetIdsByPrefix(std::string(kManagedScriptProjectPrefix)))
    {
        ManagedScriptProject project;
        project.Asset = assetId;
        project.Path = assets.ResolvePath(assetId);
        project.Exists = std::filesystem::exists(project.Path);
        project.Name = assetId.Value.rfind(kManagedScriptProjectPrefix, 0) == 0
            ? assetId.Value.substr(kManagedScriptProjectPrefix.size())
            : project.Path.stem().string();
        project.RuntimeConfigPath = project.Path.parent_path() / (project.Path.stem().string() + ".runtimeconfig.json");
        project.HasRuntimeConfig = std::filesystem::exists(project.RuntimeConfigPath);
        project.AssemblyPath = project.Path.parent_path() / (project.Path.stem().string() + ".dll");
        project.HasAssembly = std::filesystem::exists(project.AssemblyPath);

        m_projects.push_back(std::move(project));
    }

    std::sort(
        m_projects.begin(),
        m_projects.end(),
        [](const ManagedScriptProject& lhs, const ManagedScriptProject& rhs)
        {
            return lhs.Name < rhs.Name;
        });

    const bool hostFxrLoaded = m_hostFxr.Load();
    m_initialized = true;

    Pragma::Core::Log(
        Pragma::Core::LogCategory::General,
        Pragma::Core::LogLevel::Info,
        "Managed scripting host initialized. Discovered " + std::to_string(m_projects.size()) + " C# project(s).");

    Pragma::Core::Log(
        Pragma::Core::LogCategory::General,
        hostFxrLoaded ? Pragma::Core::LogLevel::Info : Pragma::Core::LogLevel::Warning,
        "Managed runtime bootstrap: " + m_hostFxr.GetStatusText() +
        (m_hostFxr.GetLibraryPath().empty() ? "" : " (" + m_hostFxr.GetLibraryPath().string() + ")"));

    for (ManagedScriptProject& project : m_projects)
    {
        if (hostFxrLoaded && project.HasRuntimeConfig)
        {
            project.RuntimeReady = m_hostFxr.ProbeRuntimeConfig(project.RuntimeConfigPath, project.RuntimeStatus);
        }
        else if (!project.HasRuntimeConfig)
        {
            project.RuntimeStatus = "runtimeconfig.json is missing.";
        }
        else
        {
            project.RuntimeStatus = m_hostFxr.GetStatusText();
        }

        Pragma::Core::Log(
            project.Exists ? Pragma::Core::LogCategory::General : Pragma::Core::LogCategory::Assets,
            project.Exists ? Pragma::Core::LogLevel::Info : Pragma::Core::LogLevel::Warning,
            "Managed project '" + project.Asset.Value + "' -> " + project.Path.string() +
            (project.Exists ? "" : " (missing)"));

        Pragma::Core::Log(
            project.RuntimeReady ? Pragma::Core::LogCategory::General : Pragma::Core::LogCategory::Assets,
            project.RuntimeReady ? Pragma::Core::LogLevel::Info : Pragma::Core::LogLevel::Warning,
            "Managed runtime probe '" + project.Asset.Value + "': " + project.RuntimeStatus +
            (project.HasRuntimeConfig ? " (" + project.RuntimeConfigPath.string() + ")" : ""));
        AppendManagedProbeTrace(
            "Managed runtime probe '" + project.Asset.Value + "': " + project.RuntimeStatus);

        project.EntryPointReady = false;
        project.EntryPointProbeValue = 0;
        project.EntryPointStatus = project.HasAssembly
            ? "managed entry point probe has not run yet."
            : "managed assembly is missing.";
        project.BindingReady = false;
        project.BindingProbeValue = 0;
        project.BindingStatus = project.RuntimeReady
            ? (project.HasAssembly ? "binding probe has not run yet." : "managed assembly is missing.")
            : "runtime bootstrap is not ready.";
        project.ScriptApiReady = false;
        project.ScriptApiStatus = project.RuntimeReady
            ? (project.HasAssembly ? "managed script api has not been resolved yet." : "managed assembly is missing.")
            : "runtime bootstrap is not ready.";

        if (project.RuntimeReady && project.HasAssembly)
        {
            ManagedScriptApi api{};
            api.ProjectAsset = project.Asset;
            project.ScriptApiReady = m_hostFxr.LoadManagedScriptApi(
                project.RuntimeConfigPath,
                project.AssemblyPath,
                L"Pragma.Managed.Bootstrap, Pragma.Managed",
                &api.CreateInstance,
                &api.StartInstance,
                &api.UpdateInstance,
                &api.DestroyInstance,
                project.ScriptApiStatus);
            if (project.ScriptApiReady)
            {
                m_scriptApis.push_back(api);
                project.EntryPointReady = true;
                project.EntryPointStatus = "managed entry point covered by managed script api resolution.";
                project.BindingReady = true;
                project.BindingStatus = "managed bindings covered by managed script api resolution.";

                if (project.Asset.Value == "script_project.pragma_managed")
                {
                    m_scriptTypes.push_back(
                        {
                            project.Asset,
                            "Pragma.Managed.Scripts.FloatUpManagedScript",
                            "Float Up Managed Script",
                            "Moves the object gently up and down from C# using managed lifecycle callbacks."
                        });
                }
            }
        }

        Pragma::Core::Log(
            project.ScriptApiReady ? Pragma::Core::LogCategory::General : Pragma::Core::LogCategory::Assets,
            project.ScriptApiReady ? Pragma::Core::LogLevel::Info : Pragma::Core::LogLevel::Warning,
            "Managed script api '" + project.Asset.Value + "': " + project.ScriptApiStatus +
            (project.HasAssembly ? " (" + project.AssemblyPath.string() + ")" : ""));
        AppendManagedProbeTrace(
            "Managed script api '" + project.Asset.Value + "': " + project.ScriptApiStatus);
    }
}

void ManagedScriptHost::RunBindingProbe(Pragma::Renderer::Scene& scene)
{
    for (ManagedScriptProject& project : m_projects)
    {
        project.EntryPointReady = false;
        project.EntryPointProbeValue = 0;
        project.BindingReady = false;
        project.BindingProbeValue = 0;

        if (!project.RuntimeReady)
        {
            project.BindingStatus = "runtime bootstrap is not ready.";
            project.EntryPointStatus = project.BindingStatus;
        }
        else if (!project.HasAssembly)
        {
            project.BindingStatus = "managed assembly is missing.";
            project.EntryPointStatus = project.BindingStatus;
        }
        else
        {
            void* functionPointer = nullptr;
            project.BindingReady = m_hostFxr.LoadUnmanagedFunctionPointer(
                project.RuntimeConfigPath,
                project.AssemblyPath,
                L"Pragma.Managed.Bootstrap, Pragma.Managed",
                L"RunManagedBindingProbe",
                &functionPointer,
                project.BindingStatus);

            if (project.BindingReady && functionPointer != nullptr)
            {
                const auto bindingProbe = reinterpret_cast<managed_binding_probe_fn>(functionPointer);
                const ManagedTimeSnapshot timeSnapshot
                {
                    0.0f,
                    scene.GetElapsedSeconds(),
                    scene.GetFrameIndex()
                };
                const ManagedBindings bindings
                {
                    reinterpret_cast<void*>(&ManagedProbeLogCallback),
                    reinterpret_cast<void*>(&ManagedFindEntityByNameCallback),
                    reinterpret_cast<void*>(&ManagedIsEntityValidCallback),
                    reinterpret_cast<void*>(&ManagedGetEntityNameCallback),
                    reinterpret_cast<void*>(&ManagedGetParentCallback),
                    reinterpret_cast<void*>(&ManagedGetChildCountCallback),
                    reinterpret_cast<void*>(&ManagedGetChildAtCallback),
                    reinterpret_cast<void*>(&ManagedGetActiveCameraEntityCallback),
                    reinterpret_cast<void*>(&ManagedGetEntityCountCallback),
                    reinterpret_cast<void*>(&ManagedGetTransformCallback),
                    reinterpret_cast<void*>(&ManagedSetTransformCallback),
                    reinterpret_cast<void*>(&ManagedGetCameraCallback),
                    reinterpret_cast<void*>(&ManagedSetCameraCallback),
                    reinterpret_cast<void*>(&ManagedGetLightCallback),
                    reinterpret_cast<void*>(&ManagedSetLightCallback)
                };
                project.BindingProbeValue = bindingProbe(timeSnapshot, bindings, &scene);
                project.BindingStatus += " Value=" + std::to_string(project.BindingProbeValue) + ".";
            }

            project.EntryPointReady = project.BindingReady;
            project.EntryPointProbeValue = project.BindingProbeValue;
            project.EntryPointStatus = project.BindingReady
                ? "managed entry point resolved through entity/transform binding probe. Value=" +
                    std::to_string(project.EntryPointProbeValue) + "."
                : project.BindingStatus;
        }

        Pragma::Core::Log(
            project.EntryPointReady ? Pragma::Core::LogCategory::General : Pragma::Core::LogCategory::Assets,
            project.EntryPointReady ? Pragma::Core::LogLevel::Info : Pragma::Core::LogLevel::Warning,
            "Managed entry point probe '" + project.Asset.Value + "': " + project.EntryPointStatus +
            (project.HasAssembly ? " (" + project.AssemblyPath.string() + ")" : ""));
        AppendManagedProbeTrace(
            "Managed entry point probe '" + project.Asset.Value + "': " + project.EntryPointStatus);

        Pragma::Core::Log(
            project.BindingReady ? Pragma::Core::LogCategory::General : Pragma::Core::LogCategory::Assets,
            project.BindingReady ? Pragma::Core::LogLevel::Info : Pragma::Core::LogLevel::Warning,
            "Managed binding probe '" + project.Asset.Value + "': " + project.BindingStatus +
            (project.HasAssembly ? " (" + project.AssemblyPath.string() + ")" : ""));
        AppendManagedProbeTrace(
            "Managed binding probe '" + project.Asset.Value + "': " + project.BindingStatus);
    }
}

bool ManagedScriptHost::CreateScriptInstance(
    const Pragma::Assets::AssetId& projectAssetId,
    const std::string_view typeName,
    const Pragma::Renderer::EntityId entityId,
    int& instanceHandle,
    std::string& status) const
{
    instanceHandle = 0;

    const auto apiIt = std::find_if(
        m_scriptApis.begin(),
        m_scriptApis.end(),
        [&projectAssetId](const ManagedScriptApi& api)
        {
            return api.ProjectAsset.Value == projectAssetId.Value;
        });
    if (apiIt == m_scriptApis.end() || apiIt->CreateInstance == nullptr)
    {
        status = "managed script api is not available for project '" + projectAssetId.Value + "'.";
        return false;
    }

    const auto createInstance = reinterpret_cast<managed_create_script_instance_fn>(apiIt->CreateInstance);
    const std::string typeNameText(typeName);
    instanceHandle = createInstance(typeNameText.c_str(), entityId);
    if (instanceHandle <= 0)
    {
        status = "managed script instance creation failed for type '" + typeNameText + "'.";
        return false;
    }

    status = "managed script instance created.";
    return true;
}

bool ManagedScriptHost::StartScriptInstance(
    const Pragma::Assets::AssetId& projectAssetId,
    const int instanceHandle,
    Pragma::Renderer::Scene& scene,
    const ManagedScriptTimeSnapshot& time,
    std::string& status) const
{
    const auto apiIt = std::find_if(
        m_scriptApis.begin(),
        m_scriptApis.end(),
        [&projectAssetId](const ManagedScriptApi& api)
        {
            return api.ProjectAsset.Value == projectAssetId.Value;
        });
    if (apiIt == m_scriptApis.end() || apiIt->StartInstance == nullptr)
    {
        status = "managed script api is not available for project '" + projectAssetId.Value + "'.";
        return false;
    }

    const auto startInstance = reinterpret_cast<managed_script_lifecycle_fn>(apiIt->StartInstance);
    const ManagedBindings bindings
    {
        reinterpret_cast<void*>(&ManagedProbeLogCallback),
        reinterpret_cast<void*>(&ManagedFindEntityByNameCallback),
        reinterpret_cast<void*>(&ManagedIsEntityValidCallback),
        reinterpret_cast<void*>(&ManagedGetEntityNameCallback),
        reinterpret_cast<void*>(&ManagedGetParentCallback),
        reinterpret_cast<void*>(&ManagedGetChildCountCallback),
        reinterpret_cast<void*>(&ManagedGetChildAtCallback),
        reinterpret_cast<void*>(&ManagedGetActiveCameraEntityCallback),
        reinterpret_cast<void*>(&ManagedGetEntityCountCallback),
        reinterpret_cast<void*>(&ManagedGetTransformCallback),
        reinterpret_cast<void*>(&ManagedSetTransformCallback),
        reinterpret_cast<void*>(&ManagedGetCameraCallback),
        reinterpret_cast<void*>(&ManagedSetCameraCallback),
        reinterpret_cast<void*>(&ManagedGetLightCallback),
        reinterpret_cast<void*>(&ManagedSetLightCallback)
    };
    const int result = startInstance(instanceHandle, ToManagedTimeSnapshot(time), bindings, &scene);
    status = "managed script OnStart returned " + std::to_string(result) + '.';
    return result != 0;
}

bool ManagedScriptHost::UpdateScriptInstance(
    const Pragma::Assets::AssetId& projectAssetId,
    const int instanceHandle,
    Pragma::Renderer::Scene& scene,
    const ManagedScriptTimeSnapshot& time,
    std::string& status) const
{
    const auto apiIt = std::find_if(
        m_scriptApis.begin(),
        m_scriptApis.end(),
        [&projectAssetId](const ManagedScriptApi& api)
        {
            return api.ProjectAsset.Value == projectAssetId.Value;
        });
    if (apiIt == m_scriptApis.end() || apiIt->UpdateInstance == nullptr)
    {
        status = "managed script api is not available for project '" + projectAssetId.Value + "'.";
        return false;
    }

    const auto updateInstance = reinterpret_cast<managed_script_lifecycle_fn>(apiIt->UpdateInstance);
    const ManagedBindings bindings
    {
        reinterpret_cast<void*>(&ManagedProbeLogCallback),
        reinterpret_cast<void*>(&ManagedFindEntityByNameCallback),
        reinterpret_cast<void*>(&ManagedIsEntityValidCallback),
        reinterpret_cast<void*>(&ManagedGetEntityNameCallback),
        reinterpret_cast<void*>(&ManagedGetParentCallback),
        reinterpret_cast<void*>(&ManagedGetChildCountCallback),
        reinterpret_cast<void*>(&ManagedGetChildAtCallback),
        reinterpret_cast<void*>(&ManagedGetActiveCameraEntityCallback),
        reinterpret_cast<void*>(&ManagedGetEntityCountCallback),
        reinterpret_cast<void*>(&ManagedGetTransformCallback),
        reinterpret_cast<void*>(&ManagedSetTransformCallback),
        reinterpret_cast<void*>(&ManagedGetCameraCallback),
        reinterpret_cast<void*>(&ManagedSetCameraCallback),
        reinterpret_cast<void*>(&ManagedGetLightCallback),
        reinterpret_cast<void*>(&ManagedSetLightCallback)
    };
    const int result = updateInstance(instanceHandle, ToManagedTimeSnapshot(time), bindings, &scene);
    status = "managed script OnUpdate returned " + std::to_string(result) + '.';
    return result != 0;
}

bool ManagedScriptHost::DestroyScriptInstance(
    const Pragma::Assets::AssetId& projectAssetId,
    const int instanceHandle,
    Pragma::Renderer::Scene& scene,
    const ManagedScriptTimeSnapshot& time,
    std::string& status) const
{
    const auto apiIt = std::find_if(
        m_scriptApis.begin(),
        m_scriptApis.end(),
        [&projectAssetId](const ManagedScriptApi& api)
        {
            return api.ProjectAsset.Value == projectAssetId.Value;
        });
    if (apiIt == m_scriptApis.end() || apiIt->DestroyInstance == nullptr)
    {
        status = "managed script api is not available for project '" + projectAssetId.Value + "'.";
        return false;
    }

    const auto destroyInstance = reinterpret_cast<managed_script_lifecycle_fn>(apiIt->DestroyInstance);
    const ManagedBindings bindings
    {
        reinterpret_cast<void*>(&ManagedProbeLogCallback),
        reinterpret_cast<void*>(&ManagedFindEntityByNameCallback),
        reinterpret_cast<void*>(&ManagedIsEntityValidCallback),
        reinterpret_cast<void*>(&ManagedGetEntityNameCallback),
        reinterpret_cast<void*>(&ManagedGetParentCallback),
        reinterpret_cast<void*>(&ManagedGetChildCountCallback),
        reinterpret_cast<void*>(&ManagedGetChildAtCallback),
        reinterpret_cast<void*>(&ManagedGetActiveCameraEntityCallback),
        reinterpret_cast<void*>(&ManagedGetEntityCountCallback),
        reinterpret_cast<void*>(&ManagedGetTransformCallback),
        reinterpret_cast<void*>(&ManagedSetTransformCallback),
        reinterpret_cast<void*>(&ManagedGetCameraCallback),
        reinterpret_cast<void*>(&ManagedSetCameraCallback),
        reinterpret_cast<void*>(&ManagedGetLightCallback),
        reinterpret_cast<void*>(&ManagedSetLightCallback)
    };
    const int result = destroyInstance(instanceHandle, ToManagedTimeSnapshot(time), bindings, &scene);
    status = "managed script OnDestroy returned " + std::to_string(result) + '.';
    return result != 0;
}

void ManagedScriptHost::Shutdown() noexcept
{
    m_projects.clear();
    m_scriptApis.clear();
    m_hostFxr.Unload();
    m_initialized = false;
}

bool ManagedScriptHost::IsInitialized() const noexcept
{
    return m_initialized;
}

std::size_t ManagedScriptHost::GetProjectCount() const noexcept
{
    return m_projects.size();
}

std::size_t ManagedScriptHost::GetRuntimeReadyProjectCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_projects.begin(),
        m_projects.end(),
        [](const ManagedScriptProject& project)
        {
            return project.RuntimeReady;
        }));
}

std::size_t ManagedScriptHost::GetEntryPointReadyProjectCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_projects.begin(),
        m_projects.end(),
        [](const ManagedScriptProject& project)
        {
            return project.EntryPointReady;
        }));
}

std::size_t ManagedScriptHost::GetBindingReadyProjectCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_projects.begin(),
        m_projects.end(),
        [](const ManagedScriptProject& project)
        {
            return project.BindingReady;
        }));
}

const std::vector<ManagedScriptProject>& ManagedScriptHost::GetProjects() const noexcept
{
    return m_projects;
}

const std::vector<ManagedScriptTypeMetadata>& ManagedScriptHost::GetAvailableScriptTypes() const noexcept
{
    return m_scriptTypes;
}

const ManagedScriptProject* ManagedScriptHost::FindProject(const std::string_view assetId) const noexcept
{
    const auto it = std::find_if(
        m_projects.begin(),
        m_projects.end(),
        [assetId](const ManagedScriptProject& project)
        {
            return project.Asset.Value == assetId;
        });
    return it != m_projects.end() ? &(*it) : nullptr;
}

bool ManagedScriptHost::IsHostFxrAvailable() const noexcept
{
    return m_hostFxr.IsLoaded();
}

const std::filesystem::path& ManagedScriptHost::GetHostFxrPath() const noexcept
{
    return m_hostFxr.GetLibraryPath();
}

const std::string& ManagedScriptHost::GetHostFxrStatus() const noexcept
{
    return m_hostFxr.GetStatusText();
}
}
