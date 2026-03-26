#include "Pragma/Core/PrefabSerializer.h"

#include "Pragma/Core/Log.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
constexpr std::uint32_t kLatestPrefabVersion = 2;
constexpr std::uint32_t kMinimumSupportedPrefabVersion = 1;

std::string Trim(std::string value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> SplitCommaSeparated(const std::string& value)
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;

    while (std::getline(stream, part, ','))
    {
        parts.push_back(Trim(part));
    }

    return parts;
}

Pragma::Math::Vector3 ParseVector3(const std::string& value)
{
    const std::vector<std::string> parts = SplitCommaSeparated(value);
    if (parts.size() != 3)
    {
        throw std::runtime_error("Expected a 3-component vector.");
    }

    return
    {
        std::stof(parts[0]),
        std::stof(parts[1]),
        std::stof(parts[2])
    };
}

bool ParseBool(const std::string& value)
{
    const std::string trimmed = Trim(value);
    if (trimmed == "true" || trimmed == "1")
    {
        return true;
    }
    if (trimmed == "false" || trimmed == "0")
    {
        return false;
    }

    throw std::runtime_error("Expected a boolean value.");
}

Pragma::Renderer::RigidBodyMotionType ParseRigidBodyMotionType(const std::string& value)
{
    const std::string trimmed = Trim(value);
    if (trimmed == "static")
    {
        return Pragma::Renderer::RigidBodyMotionType::Static;
    }
    if (trimmed == "dynamic")
    {
        return Pragma::Renderer::RigidBodyMotionType::Dynamic;
    }
    if (trimmed == "kinematic")
    {
        return Pragma::Renderer::RigidBodyMotionType::Kinematic;
    }

    throw std::runtime_error("Expected rigid body motion type: static, dynamic or kinematic.");
}

Pragma::Renderer::RigidBodyCollisionLayer ParseRigidBodyCollisionLayer(const std::string& value)
{
    const std::string trimmed = Trim(value);
    if (trimmed == "default")
    {
        return Pragma::Renderer::RigidBodyCollisionLayer::Default;
    }
    if (trimmed == "no_collision")
    {
        return Pragma::Renderer::RigidBodyCollisionLayer::NoCollision;
    }

    throw std::runtime_error("Expected rigid body collision layer: default or no_collision.");
}

std::string FormatBool(const bool value)
{
    return value ? "true" : "false";
}

std::string FormatRigidBodyMotionType(const Pragma::Renderer::RigidBodyMotionType motionType)
{
    switch (motionType)
    {
    case Pragma::Renderer::RigidBodyMotionType::Static:
        return "static";
    case Pragma::Renderer::RigidBodyMotionType::Kinematic:
        return "kinematic";
    case Pragma::Renderer::RigidBodyMotionType::Dynamic:
    default:
        return "dynamic";
    }
}

std::string FormatRigidBodyCollisionLayer(const Pragma::Renderer::RigidBodyCollisionLayer collisionLayer)
{
    switch (collisionLayer)
    {
    case Pragma::Renderer::RigidBodyCollisionLayer::NoCollision:
        return "no_collision";
    case Pragma::Renderer::RigidBodyCollisionLayer::Default:
    default:
        return "default";
    }
}

std::string FormatVector3(const Pragma::Math::Vector3& value)
{
    return std::to_string(value.X) + "," + std::to_string(value.Y) + "," + std::to_string(value.Z);
}

std::string FormatColor3(const float color[3])
{
    return std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]);
}
}

namespace Pragma::Core
{
SerializedPrefab LoadPrefabFromFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open prefab file.");
    }

    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, "Loading prefab file: " + path.string());

    enum class ParseBlock
    {
        None,
        PrefabObject,
    };

    SerializedPrefab prefab;
    SerializedSceneObject currentObject;
    ParseBlock currentBlock = ParseBlock::None;
    bool versionDefined = false;
    std::size_t lineNumber = 0;

    auto parseError = [&](const std::string& message) -> std::runtime_error
    {
        return std::runtime_error(
            "Prefab parse error in '" + path.string() + "' at line " + std::to_string(lineNumber) + ": " + message);
    };

    auto commitCurrentBlock = [&]()
    {
        if (currentBlock == ParseBlock::PrefabObject)
        {
            if (currentObject.Id == Pragma::Renderer::InvalidEntityId)
            {
                throw parseError("prefab_object is missing an id.");
            }
            if (currentObject.Name.empty())
            {
                throw parseError("prefab_object is missing a name.");
            }
            if (currentObject.ParentId == currentObject.Id)
            {
                throw parseError("prefab_object cannot parent itself.");
            }
            if (currentObject.IsActiveCamera)
            {
                throw parseError("prefabs cannot contain an active camera.");
            }
            if (currentObject.CameraController.has_value() && !currentObject.Camera.has_value())
            {
                throw parseError("prefab_object contains CameraController without Camera.");
            }
            if (currentObject.MeshRenderer.has_value() && currentObject.MeshRenderer->MeshAsset.empty())
            {
                throw parseError("prefab_object contains MeshRenderer without mesh asset.");
            }
            if (currentObject.MeshRenderer.has_value() && currentObject.MeshRenderer->MaterialAsset.empty())
            {
                throw parseError("prefab_object contains MeshRenderer without material asset.");
            }
            prefab.Objects.push_back(currentObject);
            currentObject = SerializedSceneObject{};
        }
        currentBlock = ParseBlock::None;
    };

    std::string line;
    while (std::getline(input, line))
    {
        ++lineNumber;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        if (line == "prefab_object")
        {
            if (!versionDefined)
            {
                throw parseError("version must be declared before prefab_object blocks.");
            }
            commitCurrentBlock();
            currentBlock = ParseBlock::PrefabObject;
            continue;
        }

        if (line == "end")
        {
            commitCurrentBlock();
            continue;
        }

        const std::size_t separatorIndex = line.find('=');
        if (separatorIndex == std::string::npos)
        {
            throw parseError("line is missing '=' separator.");
        }

        const std::string key = Trim(line.substr(0, separatorIndex));
        const std::string value = Trim(line.substr(separatorIndex + 1));

        if (currentBlock == ParseBlock::None)
        {
            if (key == "version")
            {
                const std::uint32_t version = static_cast<std::uint32_t>(std::stoul(value));
                if (version < kMinimumSupportedPrefabVersion || version > kLatestPrefabVersion)
                {
                    throw parseError("unsupported prefab version " + std::to_string(version) + '.');
                }

                prefab.Version = version;
                versionDefined = true;
            }
            else
            {
                throw parseError("unexpected key outside of a block: '" + key + "'.");
            }
        }
        else
        {
            auto ensureMeshRenderer = [&]() -> SerializedMeshRenderer&
            {
                if (!currentObject.MeshRenderer.has_value())
                {
                    currentObject.MeshRenderer.emplace();
                }

                return *currentObject.MeshRenderer;
            };

            auto ensureCamera = [&]() -> Pragma::Renderer::CameraComponent&
            {
                if (!currentObject.Camera.has_value())
                {
                    currentObject.Camera.emplace();
                }

                return *currentObject.Camera;
            };

            auto ensureCameraController = [&]() -> Pragma::Renderer::CameraControllerComponent&
            {
                if (!currentObject.CameraController.has_value())
                {
                    currentObject.CameraController.emplace();
                }

                return *currentObject.CameraController;
            };

            auto ensureLight = [&]() -> Pragma::Renderer::LightComponent&
            {
                if (!currentObject.Light.has_value())
                {
                    currentObject.Light.emplace();
                }

                return *currentObject.Light;
            };

            auto ensureRigidBody = [&]() -> Pragma::Renderer::RigidBodyComponent&
            {
                if (!currentObject.RigidBody.has_value())
                {
                    currentObject.RigidBody.emplace();
                }

                return *currentObject.RigidBody;
            };

            auto ensureBoxCollider = [&]() -> Pragma::Renderer::BoxColliderComponent&
            {
                if (!currentObject.BoxCollider.has_value())
                {
                    currentObject.BoxCollider.emplace();
                }

                return *currentObject.BoxCollider;
            };

            if (key == "name")
            {
                currentObject.Name = value;
            }
            else if (key == "id")
            {
                currentObject.Id = static_cast<Pragma::Renderer::EntityId>(std::stoull(value));
            }
            else if (key == "parent")
            {
                currentObject.ParentId = static_cast<Pragma::Renderer::EntityId>(std::stoull(value));
            }
            else if (key == "position")
            {
                currentObject.Transform.Position = ParseVector3(value);
            }
            else if (key == "scale")
            {
                currentObject.Transform.Scale = ParseVector3(value);
            }
            else if (key == "rotation")
            {
                currentObject.Transform.RotationRadians = ParseVector3(value);
            }
            else if (key == "yaw")
            {
                currentObject.Transform.RotationRadians.Y = std::stof(value);
            }
            else if (key == "mesh.asset")
            {
                ensureMeshRenderer().MeshAsset.Value = value;
            }
            else if (key == "mesh.lod1.asset")
            {
                ensureMeshRenderer().MediumLodMeshAsset.Value = value;
            }
            else if (key == "mesh.lod2.asset")
            {
                ensureMeshRenderer().LowLodMeshAsset.Value = value;
            }
            else if (key == "material.asset")
            {
                ensureMeshRenderer().MaterialAsset.Value = value;
            }
            else if (key == "camera.pitch")
            {
                ensureCamera().PitchRadians = std::stof(value);
            }
            else if (key == "camera.fov")
            {
                ensureCamera().FieldOfViewRadians = std::stof(value);
            }
            else if (key == "camera.near")
            {
                ensureCamera().NearPlane = std::stof(value);
            }
            else if (key == "camera.far")
            {
                ensureCamera().FarPlane = std::stof(value);
            }
            else if (key == "camera_controller.enabled")
            {
                ensureCameraController().Enabled = ParseBool(value);
            }
            else if (key == "camera_controller.move_speed")
            {
                ensureCameraController().MoveSpeed = std::stof(value);
            }
            else if (key == "camera_controller.fast_move_speed")
            {
                ensureCameraController().FastMoveSpeed = std::stof(value);
            }
            else if (key == "camera_controller.look_speed")
            {
                ensureCameraController().KeyboardLookSpeed = std::stof(value);
            }
            else if (key == "camera_controller.mouse_sensitivity")
            {
                ensureCameraController().MouseLookSensitivity = std::stof(value);
            }
            else if (key == "script")
            {
                currentObject.ScriptName = value;
            }
            else if (key == "light.direction")
            {
                const Pragma::Math::Vector3 direction = ParseVector3(value);
                ensureLight().Direction[0] = direction.X;
                ensureLight().Direction[1] = direction.Y;
                ensureLight().Direction[2] = direction.Z;
            }
            else if (key == "light.intensity")
            {
                ensureLight().Intensity = std::stof(value);
            }
            else if (key == "light.color")
            {
                const std::vector<std::string> parts = SplitCommaSeparated(value);
                if (parts.size() != 3)
                {
                    throw parseError("Expected a 3-component light color.");
                }

                ensureLight().Color[0] = std::stof(parts[0]);
                ensureLight().Color[1] = std::stof(parts[1]);
                ensureLight().Color[2] = std::stof(parts[2]);
            }
            else if (key == "rigid_body.enabled")
            {
                ensureRigidBody().Enabled = ParseBool(value);
            }
            else if (key == "rigid_body.motion_type")
            {
                ensureRigidBody().MotionType = ParseRigidBodyMotionType(value);
            }
            else if (key == "rigid_body.collision_layer")
            {
                ensureRigidBody().CollisionLayer = ParseRigidBodyCollisionLayer(value);
            }
            else if (key == "rigid_body.friction")
            {
                ensureRigidBody().Friction = std::stof(value);
            }
            else if (key == "rigid_body.restitution")
            {
                ensureRigidBody().Restitution = std::stof(value);
            }
            else if (key == "rigid_body.linear_damping")
            {
                ensureRigidBody().LinearDamping = std::stof(value);
            }
            else if (key == "rigid_body.angular_damping")
            {
                ensureRigidBody().AngularDamping = std::stof(value);
            }
            else if (key == "rigid_body.gravity_factor")
            {
                ensureRigidBody().GravityFactor = std::stof(value);
            }
            else if (key == "box_collider.half_extent")
            {
                ensureBoxCollider().HalfExtent = ParseVector3(value);
            }
            else
            {
                throw parseError("unknown prefab_object key '" + key + "'.");
            }
        }
    }

    commitCurrentBlock();

    if (!versionDefined)
    {
        throw std::runtime_error("Prefab parse error in '" + path.string() + "': missing required version.");
    }

    if (prefab.Objects.empty())
    {
        throw std::runtime_error("Prefab file does not contain any objects.");
    }

    std::size_t rootCount = 0;
    std::size_t lightCount = 0;
    std::vector<Pragma::Renderer::EntityId> ids;
    ids.reserve(prefab.Objects.size());
    for (const SerializedSceneObject& object : prefab.Objects)
    {
        if (object.ParentId == Pragma::Renderer::InvalidEntityId)
        {
            ++rootCount;
        }
        if (object.Light.has_value())
        {
            ++lightCount;
        }
        if (object.RigidBody.has_value() != object.BoxCollider.has_value())
        {
            throw std::runtime_error(
                "Prefab parse error in '" + path.string() + "': rigid body and box collider must be added together for this build.");
        }
        if (std::find(ids.begin(), ids.end(), object.Id) != ids.end())
        {
            throw std::runtime_error("Prefab parse error in '" + path.string() + "': duplicate object id detected.");
        }

        ids.push_back(object.Id);
    }

    if (rootCount != 1)
    {
        throw std::runtime_error("Prefab parse error in '" + path.string() + "': expected exactly one root object.");
    }

    if (lightCount > 1)
    {
        throw std::runtime_error("Prefab parse error in '" + path.string() + "': expected at most one directional light component.");
    }

    for (const SerializedSceneObject& object : prefab.Objects)
    {
        if (object.ParentId == Pragma::Renderer::InvalidEntityId)
        {
            continue;
        }

        const bool parentExists = std::find(ids.begin(), ids.end(), object.ParentId) != ids.end();
        if (!parentExists)
        {
            throw std::runtime_error(
                "Prefab parse error in '" + path.string() + "': object '" + object.Name + "' references a missing parent.");
        }
    }

    for (const SerializedSceneObject& object : prefab.Objects)
    {
        std::vector<Pragma::Renderer::EntityId> visited;
        Pragma::Renderer::EntityId currentParent = object.ParentId;
        while (currentParent != Pragma::Renderer::InvalidEntityId)
        {
            if (std::find(visited.begin(), visited.end(), currentParent) != visited.end() || currentParent == object.Id)
            {
                throw std::runtime_error(
                    "Prefab parse error in '" + path.string() + "': hierarchy cycle detected.");
            }

            visited.push_back(currentParent);
            const SerializedSceneObject* parentObject = nullptr;
            for (const SerializedSceneObject& candidate : prefab.Objects)
            {
                if (candidate.Id == currentParent)
                {
                    parentObject = &candidate;
                    break;
                }
            }

            currentParent = parentObject != nullptr ? parentObject->ParentId : Pragma::Renderer::InvalidEntityId;
        }
    }

    return prefab;
}

void SavePrefabToFile(const SerializedPrefab& prefab, const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error("Failed to open prefab file for writing.");
    }

    output << "# Pragma prefab file\n";
    output << "version=" << kLatestPrefabVersion << "\n\n";

    for (const SerializedSceneObject& object : prefab.Objects)
    {
        output << "prefab_object\n";
        output << "id=" << object.Id << '\n';
        if (object.ParentId != Pragma::Renderer::InvalidEntityId)
        {
            output << "parent=" << object.ParentId << '\n';
        }
        output << "name=" << object.Name << '\n';
        output << "position=" << FormatVector3(object.Transform.Position) << '\n';
        output << "scale=" << FormatVector3(object.Transform.Scale) << '\n';
        output << "rotation=" << FormatVector3(object.Transform.RotationRadians) << '\n';

        if (object.MeshRenderer.has_value())
        {
            output << "mesh.asset=" << object.MeshRenderer->MeshAsset.Value << '\n';
            if (!object.MeshRenderer->MediumLodMeshAsset.empty())
            {
                output << "mesh.lod1.asset=" << object.MeshRenderer->MediumLodMeshAsset.Value << '\n';
            }
            if (!object.MeshRenderer->LowLodMeshAsset.empty())
            {
                output << "mesh.lod2.asset=" << object.MeshRenderer->LowLodMeshAsset.Value << '\n';
            }
            output << "material.asset=" << object.MeshRenderer->MaterialAsset.Value << '\n';
        }

        if (object.Camera.has_value())
        {
            output << "camera.pitch=" << object.Camera->PitchRadians << '\n';
            output << "camera.fov=" << object.Camera->FieldOfViewRadians << '\n';
            output << "camera.near=" << object.Camera->NearPlane << '\n';
            output << "camera.far=" << object.Camera->FarPlane << '\n';
        }

        if (object.CameraController.has_value())
        {
            output << "camera_controller.enabled=" << FormatBool(object.CameraController->Enabled) << '\n';
            output << "camera_controller.move_speed=" << object.CameraController->MoveSpeed << '\n';
            output << "camera_controller.fast_move_speed=" << object.CameraController->FastMoveSpeed << '\n';
            output << "camera_controller.look_speed=" << object.CameraController->KeyboardLookSpeed << '\n';
            output << "camera_controller.mouse_sensitivity=" << object.CameraController->MouseLookSensitivity << '\n';
        }

        if (!object.ScriptName.empty())
        {
            output << "script=" << object.ScriptName << '\n';
        }

        if (object.Light.has_value())
        {
            output << "light.direction=" << object.Light->Direction[0] << ',' << object.Light->Direction[1] << ',' << object.Light->Direction[2] << '\n';
            output << "light.intensity=" << object.Light->Intensity << '\n';
            output << "light.color=" << FormatColor3(object.Light->Color) << '\n';
        }

        if (object.RigidBody.has_value())
        {
            output << "rigid_body.enabled=" << FormatBool(object.RigidBody->Enabled) << '\n';
            output << "rigid_body.motion_type=" << FormatRigidBodyMotionType(object.RigidBody->MotionType) << '\n';
            output << "rigid_body.collision_layer=" << FormatRigidBodyCollisionLayer(object.RigidBody->CollisionLayer) << '\n';
            output << "rigid_body.friction=" << object.RigidBody->Friction << '\n';
            output << "rigid_body.restitution=" << object.RigidBody->Restitution << '\n';
            output << "rigid_body.linear_damping=" << object.RigidBody->LinearDamping << '\n';
            output << "rigid_body.angular_damping=" << object.RigidBody->AngularDamping << '\n';
            output << "rigid_body.gravity_factor=" << object.RigidBody->GravityFactor << '\n';
        }

        if (object.BoxCollider.has_value())
        {
            output << "box_collider.half_extent=" << FormatVector3(object.BoxCollider->HalfExtent) << '\n';
        }

        output << "end\n\n";
    }
}
}
