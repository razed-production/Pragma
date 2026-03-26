#pragma once

#include "Pragma/Core/EngineTime.h"
#include "Pragma/Renderer/Entity.h"

#include <memory>

namespace Pragma::Renderer
{
class Scene;
}

namespace Pragma::Physics
{
class PhysicsSystem
{
public:
    struct BodyDebugState
    {
        bool HasBody = false;
        bool IsActive = false;
    };

    PhysicsSystem();
    ~PhysicsSystem();

    PhysicsSystem(const PhysicsSystem&) = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;

    void Initialize();
    void Shutdown();
    void Update(Pragma::Renderer::Scene& scene, const Pragma::Core::EngineTime& time);
    [[nodiscard]] BodyDebugState GetBodyDebugState(Pragma::Renderer::EntityId entityId);
    [[nodiscard]] std::size_t GetBodyCount() const noexcept;
    [[nodiscard]] std::size_t GetActiveBodyCount();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}
