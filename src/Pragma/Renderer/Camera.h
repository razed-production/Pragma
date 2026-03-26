#pragma once

#include "Pragma/Math/Matrix4.h"

namespace Pragma::Renderer
{
class Camera
{
public:
    void SetPerspective(float fovYRadians, float aspectRatio, float nearPlane, float farPlane) noexcept;
    void SetPose(const Pragma::Math::Vector3& position, float yawRadians, float pitchRadians) noexcept;
    void MoveLocal(float forward, float right, float up) noexcept;
    void Rotate(float deltaYawRadians, float deltaPitchRadians) noexcept;

    [[nodiscard]] Pragma::Math::Matrix4 GetViewProjection() const noexcept;
    [[nodiscard]] Pragma::Math::Vector3 GetPosition() const noexcept;
    [[nodiscard]] float GetYawRadians() const noexcept;
    [[nodiscard]] float GetPitchRadians() const noexcept;

private:
    Pragma::Math::Matrix4 m_projection = Pragma::Math::Identity();
    Pragma::Math::Vector3 m_position{ 0.0f, 0.0f, 0.0f };
    float m_yawRadians = 0.0f;
    float m_pitchRadians = 0.0f;
};
}
