#pragma once

#include "Pragma/Core/SceneSerializer.h"
#include "Pragma/Renderer/Scene.h"

#include <string>

namespace Pragma::Core
{
class SceneStateBridge
{
public:
    [[nodiscard]] static SerializedScene Capture(
        const Pragma::Renderer::Scene& scene,
        const SerializedScene& serializedScene);

    [[nodiscard]] static bool AreEqual(const SerializedScene& lhs, const SerializedScene& rhs) noexcept;
    [[nodiscard]] static std::string DescribeDifference(const SerializedScene& expected, const SerializedScene& actual);
};
}
