#pragma once

#include "Pragma/Core/Assert.h"
#include "Pragma/Renderer/BehaviourComponent.h"
#include "Pragma/Renderer/ScriptableEntity.h"

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Pragma::Renderer
{
class NativeScriptComponent final : public BehaviourComponent
{
public:
    using ScriptFactory = std::function<std::unique_ptr<ScriptableEntity>()>;

    NativeScriptComponent() = default;
    ~NativeScriptComponent() override = default;

    template <typename TScript, typename... TArgs>
    void Bind(TArgs&&... args)
    {
        static_assert(std::is_base_of_v<ScriptableEntity, TScript>, "TScript must derive from ScriptableEntity.");
        auto arguments = std::make_tuple(std::forward<TArgs>(args)...);
        m_factory = [arguments = std::move(arguments)]() mutable
        {
            return std::apply(
                [](auto&&... unpackedArgs)
                {
                    return std::make_unique<TScript>(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },
                std::move(arguments));
        };
        m_instance.reset();
    }

    void BindFactory(ScriptFactory factory)
    {
        PRAGMA_ASSERT(static_cast<bool>(factory), "NativeScriptComponent requires a valid script factory.");
        m_factory = std::move(factory);
        m_instance.reset();
    }

    void ClearBinding() noexcept
    {
        m_factory = {};
        m_instance.reset();
        m_scriptName.clear();
    }

    [[nodiscard]] bool HasScriptBound() const noexcept
    {
        return static_cast<bool>(m_factory);
    }

    [[nodiscard]] ScriptableEntity* GetInstance() noexcept
    {
        return m_instance.get();
    }

    [[nodiscard]] const ScriptableEntity* GetInstance() const noexcept
    {
        return m_instance.get();
    }

    void SetScriptName(std::string scriptName)
    {
        m_scriptName = std::move(scriptName);
    }

    [[nodiscard]] const std::string& GetScriptName() const noexcept
    {
        return m_scriptName;
    }

    [[nodiscard]] const char* GetComponentName() const noexcept override
    {
        return "Native Script";
    }

    void OnStart(const BehaviourContext& context) override
    {
        ScriptableEntity& script = EnsureInstance();
        script.Bind(context.World, context.Entity, context.Time, context.Input);
        script.OnStart();
        script.Unbind();
    }

    void OnUpdate(const BehaviourContext& context) override
    {
        ScriptableEntity& script = EnsureInstance();
        script.Bind(context.World, context.Entity, context.Time, context.Input);
        script.OnUpdate();
        script.Unbind();
    }

    void OnDestroy(const BehaviourContext& context) override
    {
        if (m_instance == nullptr)
        {
            return;
        }

        m_instance->Bind(context.World, context.Entity, context.Time, context.Input);
        m_instance->OnDestroy();
        m_instance->Unbind();
    }

private:
    ScriptableEntity& EnsureInstance()
    {
        PRAGMA_ASSERT(static_cast<bool>(m_factory), "NativeScriptComponent is missing a bound script type.");

        if (m_instance == nullptr)
        {
            m_instance = m_factory();
        }

        return *m_instance;
    }

private:
    ScriptFactory m_factory;
    std::unique_ptr<ScriptableEntity> m_instance;
    std::string m_scriptName;
};
}
