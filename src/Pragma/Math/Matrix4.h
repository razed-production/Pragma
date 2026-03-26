#pragma once

#include "Pragma/Math/Vector3.h"

#include <cmath>

namespace Pragma::Math
{
struct Matrix4
{
    float M[4][4]{};
};

[[nodiscard]] inline Matrix4 Identity() noexcept
{
    Matrix4 result{};
    result.M[0][0] = 1.0f;
    result.M[1][1] = 1.0f;
    result.M[2][2] = 1.0f;
    result.M[3][3] = 1.0f;
    return result;
}

[[nodiscard]] inline Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs) noexcept
{
    Matrix4 result{};

    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            result.M[row][column] =
                lhs.M[row][0] * rhs.M[0][column] +
                lhs.M[row][1] * rhs.M[1][column] +
                lhs.M[row][2] * rhs.M[2][column] +
                lhs.M[row][3] * rhs.M[3][column];
        }
    }

    return result;
}

[[nodiscard]] inline Matrix4 RotationY(const float radians) noexcept
{
    Matrix4 result = Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    result.M[0][0] = c;
    result.M[0][2] = s;
    result.M[2][0] = -s;
    result.M[2][2] = c;
    return result;
}

[[nodiscard]] inline Matrix4 RotationX(const float radians) noexcept
{
    Matrix4 result = Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    result.M[1][1] = c;
    result.M[1][2] = -s;
    result.M[2][1] = s;
    result.M[2][2] = c;
    return result;
}

[[nodiscard]] inline Matrix4 RotationZ(const float radians) noexcept
{
    Matrix4 result = Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    result.M[0][0] = c;
    result.M[0][1] = -s;
    result.M[1][0] = s;
    result.M[1][1] = c;
    return result;
}

[[nodiscard]] inline Matrix4 Scale(const Vector3& scale) noexcept
{
    Matrix4 result = Identity();
    result.M[0][0] = scale.X;
    result.M[1][1] = scale.Y;
    result.M[2][2] = scale.Z;
    return result;
}

[[nodiscard]] inline Matrix4 Translation(const Vector3& translation) noexcept
{
    Matrix4 result = Identity();
    result.M[3][0] = translation.X;
    result.M[3][1] = translation.Y;
    result.M[3][2] = translation.Z;
    return result;
}

[[nodiscard]] inline Matrix4 PerspectiveFovRH(const float fovYRadians, const float aspectRatio, const float nearPlane, const float farPlane) noexcept
{
    Matrix4 result{};
    const float yScale = 1.0f / std::tan(fovYRadians * 0.5f);
    const float xScale = yScale / aspectRatio;

    result.M[0][0] = xScale;
    result.M[1][1] = yScale;
    result.M[2][2] = farPlane / (nearPlane - farPlane);
    result.M[2][3] = -1.0f;
    result.M[3][2] = (nearPlane * farPlane) / (nearPlane - farPlane);
    return result;
}

[[nodiscard]] inline Matrix4 OrthographicRH(const float width, const float height, const float nearPlane, const float farPlane) noexcept
{
    Matrix4 result = Identity();
    result.M[0][0] = 2.0f / width;
    result.M[1][1] = 2.0f / height;
    result.M[2][2] = 1.0f / (nearPlane - farPlane);
    result.M[3][2] = nearPlane / (nearPlane - farPlane);
    return result;
}

[[nodiscard]] inline Matrix4 LookAtRH(const Vector3& eye, const Vector3& target, const Vector3& up) noexcept
{
    const Vector3 forward = Normalize(eye - target);
    const Vector3 right = Normalize(Cross(up, forward));
    const Vector3 cameraUp = Cross(forward, right);

    Matrix4 result = Identity();
    result.M[0][0] = right.X;
    result.M[1][0] = right.Y;
    result.M[2][0] = right.Z;
    result.M[0][1] = cameraUp.X;
    result.M[1][1] = cameraUp.Y;
    result.M[2][1] = cameraUp.Z;
    result.M[0][2] = forward.X;
    result.M[1][2] = forward.Y;
    result.M[2][2] = forward.Z;
    result.M[3][0] = -Dot(right, eye);
    result.M[3][1] = -Dot(cameraUp, eye);
    result.M[3][2] = -Dot(forward, eye);
    return result;
}
}
