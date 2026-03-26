#pragma once

#include "Pragma/Assets/AssetId.h"
#include "Pragma/Renderer/CameraComponent.h"
#include "Pragma/Renderer/CameraControllerComponent.h"
#include "Pragma/Renderer\Entity.h"
#include "Pragma/Physics/BoxColliderComponent.h"
#include "Pragma/Physics/RigidBodyComponent.h"
#include "Pragma/Renderer/LightComponent.h"
#include "Pragma/Renderer/Transform.h"

#include <filesystem>
#include <optional>
#include <string>
#include <cstdint>
#include <vector>

namespace Pragma::Core
{
struct SerializedMeshRenderer
{
    Pragma::Assets::AssetId MeshAsset;
    Pragma::Assets::AssetId MaterialAsset;
};

struct SerializedSceneObject
{
    Pragma::Renderer::EntityId Id = Pragma::Renderer::InvalidEntityId;
    Pragma::Renderer::EntityId ParentId = Pragma::Renderer::InvalidEntityId;
    std::string Name;
    Pragma::Assets::AssetId PrefabAssetId;
    Pragma::Renderer::Transform Transform;
    std::optional<SerializedMeshRenderer> MeshRenderer;
    std::optional<Pragma::Renderer::CameraComponent> Camera;
    std::optional<Pragma::Renderer::CameraControllerComponent> CameraController;
    std::optional<Pragma::Renderer::LightComponent> Light;
    std::optional<Pragma::Renderer::RigidBodyComponent> RigidBody;
    std::optional<Pragma::Renderer::BoxColliderComponent> BoxCollider;
    std::string ScriptName;
    Pragma::Assets::AssetId ManagedScriptProjectAsset;
    std::string ManagedScriptTypeName;
    bool IsActiveCamera = false;
};

struct SerializedScene
{
    std::uint32_t Version = 11;
    std::vector<SerializedSceneObject> Objects;
};

[[nodiscard]] SerializedScene LoadSceneFromFile(const std::filesystem::path& path);
void SaveSceneToFile(const SerializedScene& scene, const std::filesystem::path& path);
}
