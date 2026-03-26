#include "Pragma/Renderer/Camera.h"

#include <algorithm>
#include <cmath>

namespace Pragma::Renderer
{
void Camera::SetPerspective(const float fovYRadians, const float aspectRatio, const float nearPlane, const float farPlane) noexcept
{
    m_projection = Pragma::Math::PerspectiveFovRH(fovYRadians, aspectRatio, nearPlane, farPlane);
}

void Camera::SetPose(const Pragma::Math::Vector3& position, const float yawRadians, const float pitchRadians) noexcept
{
    m_position = position;
    m_yawRadians = yawRadians;
    m_pitchRadians = pitchRadians;
}

void Camera::MoveLocal(const float forward, const float right, const float up) noexcept
{
    const float sinYaw = std::sin(m_yawRadians);
    const float cosYaw = std::cos(m_yawRadians);

    const Pragma::Math::Vector3 forwardDirection{ sinYaw, 0.0f, cosYaw };
    const Pragma::Math::Vector3 rightDirection{ -cosYaw, 0.0f, sinYaw };
    const Pragma::Math::Vector3 upDirection{ 0.0f, 1.0f, 0.0f };

    m_position = m_position
        + forwardDirection * forward
        + rightDirection * right
        + upDirection * up;
}

void Camera::Rotate(const float deltaYawRadians, const float deltaPitchRadians) noexcept
{
    constexpr float kPitchLimit = 1.45f;
    m_yawRadians += deltaYawRadians;
    m_pitchRadians = std::clamp(m_pitchRadians + deltaPitchRadians, -kPitchLimit, kPitchLimit);
}

Pragma::Math::Matrix4 Camera::GetViewProjection() const noexcept
{
    const float cosPitch = std::cos(m_pitchRadians);
    const Pragma::Math::Vector3 forward
    {
        std::sin(m_yawRadians) * cosPitch,
        std::sin(m_pitchRadians),
        std::cos(m_yawRadians) * cosPitch
    };

    const Pragma::Math::Matrix4 view = Pragma::Math::LookAtRH(
        m_position,
        m_position + forward,
        { 0.0f, 1.0f, 0.0f });

    return Pragma::Math::Multiply(view, m_projection);
}

Pragma::Math::Vector3 Camera::GetPosition() const noexcept
{
    return m_position;
}

float Camera::GetYawRadians() const noexcept
{
    return m_yawRadians;
}

float Camera::GetPitchRadians() const noexcept
{
    return m_pitchRadians;
}
}
