#pragma once

#include "Pragma/Renderer/Component.h"
#include "Pragma/Renderer/BehaviourContext.h"

namespace Pragma::Renderer
{
class BehaviourComponent : public SceneComponent
{
public:
    static constexpr ComponentType kType = ComponentType::Behaviour;
    static constexpr const char* kName = "Behaviour";

    virtual ~BehaviourComponent() = default;

    virtual void OnStart(const BehaviourContext& context)
    {
        (void)context;
    }

    virtual void OnUpdate(const BehaviourContext& context)
    {
        (void)context;
    }

    virtual void OnDestroy(const BehaviourContext& context)
    {
        (void)context;
    }

    [[nodiscard]] bool HasStarted() const noexcept
    {
        return m_hasStarted;
    }

    [[nodiscard]] bool IsEnabled() const noexcept
    {
        return m_enabled;
    }

    void SetEnabled(const bool enabled) noexcept
    {
        m_enabled = enabled;
    }

    void MarkStarted() noexcept
    {
        m_hasStarted = true;
    }

    [[nodiscard]] ComponentType GetComponentType() const noexcept override
    {
        return kType;
    }

    [[nodiscard]] const char* GetComponentName() const noexcept override
    {
        return kName;
    }

private:
    bool m_hasStarted = false;
    bool m_enabled = true;
};
}
