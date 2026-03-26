#pragma once

#include <cmath>

namespace Pragma::Math
{
struct Vector3
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
};

[[nodiscard]] inline Vector3 operator+(const Vector3& lhs, const Vector3& rhs) noexcept
{
    return { lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z };
}

[[nodiscard]] inline Vector3 operator-(const Vector3& lhs, const Vector3& rhs) noexcept
{
    return { lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z };
}

[[nodiscard]] inline Vector3 operator*(const Vector3& value, const float scalar) noexcept
{
    return { value.X * scalar, value.Y * scalar, value.Z * scalar };
}

[[nodiscard]] inline Vector3 MultiplyComponents(const Vector3& lhs, const Vector3& rhs) noexcept
{
    return { lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z };
}

[[nodiscard]] inline Vector3 DivideComponents(const Vector3& lhs, const Vector3& rhs) noexcept
{
    return
    {
        rhs.X != 0.0f ? lhs.X / rhs.X : 0.0f,
        rhs.Y != 0.0f ? lhs.Y / rhs.Y : 0.0f,
        rhs.Z != 0.0f ? lhs.Z / rhs.Z : 0.0f
    };
}

[[nodiscard]] inline float Dot(const Vector3& lhs, const Vector3& rhs) noexcept
{
    return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
}

[[nodiscard]] inline Vector3 Cross(const Vector3& lhs, const Vector3& rhs) noexcept
{
    return
    {
        lhs.Y * rhs.Z - lhs.Z * rhs.Y,
        lhs.Z * rhs.X - lhs.X * rhs.Z,
        lhs.X * rhs.Y - lhs.Y * rhs.X
    };
}

[[nodiscard]] inline float Length(const Vector3& value) noexcept
{
    return std::sqrt(Dot(value, value));
}

[[nodiscard]] inline Vector3 Normalize(const Vector3& value) noexcept
{
    const float length = Length(value);
    if (length <= 0.0f)
    {
        return {};
    }

    const float invLength = 1.0f / length;
    return { value.X * invLength, value.Y * invLength, value.Z * invLength };
}
}
