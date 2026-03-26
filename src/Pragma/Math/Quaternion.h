#pragma once

#include "Pragma/Math/Vector3.h"

#include <algorithm>
#include <cmath>

namespace Pragma::Math
{
struct Quaternion
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float W = 1.0f;
};

[[nodiscard]] inline Quaternion IdentityQuaternion() noexcept
{
    return {};
}

[[nodiscard]] inline Quaternion Normalize(const Quaternion& value) noexcept
{
    const float lengthSquared = value.X * value.X + value.Y * value.Y + value.Z * value.Z + value.W * value.W;
    if (lengthSquared <= 0.0f)
    {
        return IdentityQuaternion();
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return
    {
        value.X * inverseLength,
        value.Y * inverseLength,
        value.Z * inverseLength,
        value.W * inverseLength
    };
}

[[nodiscard]] inline Quaternion Conjugate(const Quaternion& value) noexcept
{
    return { -value.X, -value.Y, -value.Z, value.W };
}

[[nodiscard]] inline Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) noexcept
{
    return
    {
        lhs.W * rhs.X + lhs.X * rhs.W + lhs.Y * rhs.Z - lhs.Z * rhs.Y,
        lhs.W * rhs.Y - lhs.X * rhs.Z + lhs.Y * rhs.W + lhs.Z * rhs.X,
        lhs.W * rhs.Z + lhs.X * rhs.Y - lhs.Y * rhs.X + lhs.Z * rhs.W,
        lhs.W * rhs.W - lhs.X * rhs.X - lhs.Y * rhs.Y - lhs.Z * rhs.Z
    };
}

[[nodiscard]] inline Quaternion QuaternionFromEulerRadians(const Vector3& radians) noexcept
{
    const float halfPitch = radians.X * 0.5f;
    const float halfYaw = radians.Y * 0.5f;
    const float halfRoll = radians.Z * 0.5f;

    const float sinPitch = std::sin(halfPitch);
    const float cosPitch = std::cos(halfPitch);
    const float sinYaw = std::sin(halfYaw);
    const float cosYaw = std::cos(halfYaw);
    const float sinRoll = std::sin(halfRoll);
    const float cosRoll = std::cos(halfRoll);

    const Quaternion pitch = { sinPitch, 0.0f, 0.0f, cosPitch };
    const Quaternion yaw = { 0.0f, sinYaw, 0.0f, cosYaw };
    const Quaternion roll = { 0.0f, 0.0f, sinRoll, cosRoll };
    return Normalize(Multiply(Multiply(pitch, yaw), roll));
}

[[nodiscard]] inline Vector3 EulerRadiansFromQuaternion(const Quaternion& quaternion) noexcept
{
    const Quaternion normalized = Normalize(quaternion);

    const float sinrCosp = 2.0f * (normalized.W * normalized.X + normalized.Y * normalized.Z);
    const float cosrCosp = 1.0f - 2.0f * (normalized.X * normalized.X + normalized.Y * normalized.Y);
    const float pitch = std::atan2(sinrCosp, cosrCosp);

    const float sinp = 2.0f * (normalized.W * normalized.Y - normalized.Z * normalized.X);
    const float yaw = std::abs(sinp) >= 1.0f
        ? std::copysign(3.1415926535f * 0.5f, sinp)
        : std::asin(sinp);

    const float sinyCosp = 2.0f * (normalized.W * normalized.Z + normalized.X * normalized.Y);
    const float cosyCosp = 1.0f - 2.0f * (normalized.Y * normalized.Y + normalized.Z * normalized.Z);
    const float roll = std::atan2(sinyCosp, cosyCosp);

    return { pitch, yaw, roll };
}

[[nodiscard]] inline Vector3 RotateVector(const Vector3& value, const Quaternion& rotation) noexcept
{
    const Quaternion vectorQuaternion{ value.X, value.Y, value.Z, 0.0f };
    const Quaternion rotated = Multiply(Multiply(rotation, vectorQuaternion), Conjugate(rotation));
    return { rotated.X, rotated.Y, rotated.Z };
}
}
