#pragma once

#include "Pragma/Core/Assert.h"
#include "Pragma/Renderer/NativeScriptComponent.h"

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace Pragma::Renderer
{
struct NativeScriptMetadata
{
    std::string Id;
    std::string DisplayName;
    std::string Description;
};

class NativeScriptRegistry
{
public:
    template <typename TScript, typename... TArgs>
    void Register(std::string id, std::string displayName, std::string description, TArgs&&... args)
    {
        static_assert(std::is_base_of_v<ScriptableEntity, TScript>, "TScript must derive from ScriptableEntity.");
        PRAGMA_ASSERT(!id.empty(), "NativeScriptRegistry requires a non-empty script id.");
        PRAGMA_ASSERT(!displayName.empty(), "NativeScriptRegistry requires a non-empty display name.");
        PRAGMA_ASSERT(!IsRegistered(id), "NativeScriptRegistry received a duplicate script registration.");

        auto arguments = std::make_tuple(std::forward<TArgs>(args)...);
        const std::string scriptId = id;
        m_factories.emplace(
            scriptId,
            [arguments = std::move(arguments), scriptId]() mutable
            {
                auto component = std::make_shared<NativeScriptComponent>();
                component->SetScriptName(scriptId);
                std::apply(
                    [&component](auto&&... unpackedArgs)
                    {
                        component->Bind<TScript>(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                    },
                    std::move(arguments));
                return component;
            });
        m_metadata.emplace_back(NativeScriptMetadata{ std::move(id), std::move(displayName), std::move(description) });
    }

    [[nodiscard]] bool IsRegistered(std::string_view name) const noexcept;
    [[nodiscard]] std::shared_ptr<NativeScriptComponent> Create(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> GetRegisteredNames() const;
    [[nodiscard]] const NativeScriptMetadata* FindMetadata(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<NativeScriptMetadata> GetRegisteredScripts() const;
    void Clear() noexcept;

private:
    using Factory = std::function<std::shared_ptr<NativeScriptComponent>()>;
    std::unordered_map<std::string, Factory> m_factories;
    std::vector<NativeScriptMetadata> m_metadata;
};
}
