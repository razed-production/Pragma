#pragma once

#include "Pragma/Math/Matrix4.h"
#include "Pragma/Math/Quaternion.h"
#include "Pragma/Math/Vector3.h"

namespace Pragma::Renderer
{
struct Transform
{
    Pragma::Math::Vector3 Position;
    Pragma::Math::Vector3 RotationRadians;
    Pragma::Math::Vector3 Scale{ 1.0f, 1.0f, 1.0f };
};

[[nodiscard]] inline Pragma::Math::Quaternion ToQuaternion(const Transform& transform) noexcept
{
    return Pragma::Math::QuaternionFromEulerRadians(transform.RotationRadians);
}

[[nodiscard]] inline Pragma::Math::Vector3 GetRightAxis(const Transform& transform) noexcept
{
    return Pragma::Math::RotateVector({ 1.0f, 0.0f, 0.0f }, ToQuaternion(transform));
}

[[nodiscard]] inline Pragma::Math::Vector3 GetUpAxis(const Transform& transform) noexcept
{
    return Pragma::Math::RotateVector({ 0.0f, 1.0f, 0.0f }, ToQuaternion(transform));
}

[[nodiscard]] inline Pragma::Math::Vector3 GetForwardAxis(const Transform& transform) noexcept
{
    return Pragma::Math::RotateVector({ 0.0f, 0.0f, 1.0f }, ToQuaternion(transform));
}

[[nodiscard]] inline Pragma::Math::Vector3 RotateVectorY(const Pragma::Math::Vector3& value, const float radians) noexcept
{
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return
    {
        value.X * c - value.Z * s,
        value.Y,
        value.X * s + value.Z * c
    };
}

[[nodiscard]] inline Transform CombineTransforms(const Transform& parent, const Transform& local) noexcept
{
    Transform result{};
    const Pragma::Math::Quaternion parentRotation = Pragma::Math::QuaternionFromEulerRadians(parent.RotationRadians);
    const Pragma::Math::Quaternion localRotation = Pragma::Math::QuaternionFromEulerRadians(local.RotationRadians);
    result.Position = parent.Position + Pragma::Math::RotateVector(
        Pragma::Math::MultiplyComponents(local.Position, parent.Scale),
        parentRotation);
    result.RotationRadians = Pragma::Math::EulerRadiansFromQuaternion(
        Pragma::Math::Normalize(Pragma::Math::Multiply(parentRotation, localRotation)));
    result.Scale = Pragma::Math::MultiplyComponents(parent.Scale, local.Scale);
    return result;
}

[[nodiscard]] inline Transform MakeRelativeTransform(const Transform& parent, const Transform& world) noexcept
{
    Transform result{};
    const Pragma::Math::Quaternion parentRotation = Pragma::Math::QuaternionFromEulerRadians(parent.RotationRadians);
    const Pragma::Math::Quaternion worldRotation = Pragma::Math::QuaternionFromEulerRadians(world.RotationRadians);
    result.Position = Pragma::Math::DivideComponents(
        Pragma::Math::RotateVector(world.Position - parent.Position, Pragma::Math::Conjugate(parentRotation)),
        parent.Scale);
    result.RotationRadians = Pragma::Math::EulerRadiansFromQuaternion(
        Pragma::Math::Normalize(Pragma::Math::Multiply(Pragma::Math::Conjugate(parentRotation), worldRotation)));
    result.Scale = Pragma::Math::DivideComponents(world.Scale, parent.Scale);
    return result;
}

[[nodiscard]] inline Pragma::Math::Matrix4 ToMatrix(const Transform& transform) noexcept
{
    const Pragma::Math::Matrix4 rotation = Pragma::Math::Multiply(
        Pragma::Math::Multiply(
            Pragma::Math::RotationX(transform.RotationRadians.X),
            Pragma::Math::RotationY(transform.RotationRadians.Y)),
        Pragma::Math::RotationZ(transform.RotationRadians.Z));
    return Pragma::Math::Multiply(
        Pragma::Math::Multiply(
            Pragma::Math::Scale(transform.Scale),
            rotation),
        Pragma::Math::Translation(transform.Position));
}
}
