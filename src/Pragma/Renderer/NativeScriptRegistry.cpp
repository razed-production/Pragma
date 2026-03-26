#include "Pragma/Renderer/NativeScriptRegistry.h"

#include <algorithm>

namespace Pragma::Renderer
{
bool NativeScriptRegistry::IsRegistered(const std::string_view name) const noexcept
{
    return m_factories.find(std::string(name)) != m_factories.end();
}

std::shared_ptr<NativeScriptComponent> NativeScriptRegistry::Create(const std::string_view name) const
{
    const auto it = m_factories.find(std::string(name));
    if (it == m_factories.end())
    {
        return nullptr;
    }

    return it->second();
}

std::vector<std::string> NativeScriptRegistry::GetRegisteredNames() const
{
    std::vector<std::string> names;
    names.reserve(m_metadata.size());
    for (const NativeScriptMetadata& metadata : m_metadata)
    {
        names.push_back(metadata.Id);
    }

    std::sort(names.begin(), names.end());
    return names;
}

const NativeScriptMetadata* NativeScriptRegistry::FindMetadata(const std::string_view id) const noexcept
{
    const auto it = std::find_if(
        m_metadata.begin(),
        m_metadata.end(),
        [id](const NativeScriptMetadata& metadata)
        {
            return metadata.Id == id;
        });
    return it != m_metadata.end() ? &(*it) : nullptr;
}

std::vector<NativeScriptMetadata> NativeScriptRegistry::GetRegisteredScripts() const
{
    std::vector<NativeScriptMetadata> scripts = m_metadata;
    std::sort(
        scripts.begin(),
        scripts.end(),
        [](const NativeScriptMetadata& lhs, const NativeScriptMetadata& rhs)
        {
            if (lhs.DisplayName == rhs.DisplayName)
            {
                return lhs.Id < rhs.Id;
            }

            return lhs.DisplayName < rhs.DisplayName;
        });
    return scripts;
}

void NativeScriptRegistry::Clear() noexcept
{
    m_factories.clear();
    m_metadata.clear();
}
}
