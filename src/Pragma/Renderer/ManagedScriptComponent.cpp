#include "Pragma/Renderer/ManagedScriptComponent.h"

#include "Pragma/Renderer/Scene.h"
#include "Pragma/Scripting/ManagedScriptHost.h"

namespace Pragma::Renderer
{
ManagedScriptComponent::ManagedScriptComponent(Pragma::Assets::AssetId projectAssetId, std::string typeName)
    : m_projectAssetId(std::move(projectAssetId))
    , m_typeName(std::move(typeName))
{
}

ComponentType ManagedScriptComponent::GetComponentType() const noexcept
{
    return kType;
}

const char* ManagedScriptComponent::GetComponentName() const noexcept
{
    return "Managed Script";
}

const Pragma::Assets::AssetId& ManagedScriptComponent::GetProjectAssetId() const noexcept
{
    return m_projectAssetId;
}

const std::string& ManagedScriptComponent::GetTypeName() const noexcept
{
    return m_typeName;
}

int ManagedScriptComponent::GetInstanceHandle() const noexcept
{
    return m_instanceHandle;
}

const std::string& ManagedScriptComponent::GetLastStatus() const noexcept
{
    return m_lastStatus;
}

void ManagedScriptComponent::SetBinding(Pragma::Assets::AssetId projectAssetId, std::string typeName)
{
    m_projectAssetId = std::move(projectAssetId);
    m_typeName = std::move(typeName);
    m_instanceHandle = 0;
    m_lastStatus.clear();
}

void ManagedScriptComponent::OnStart(const BehaviourContext& context)
{
    auto* host = context.World.GetManagedScriptHost();
    if (host == nullptr)
    {
        m_lastStatus = "managed script host is unavailable.";
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
        return;
    }

    if (m_instanceHandle == 0)
    {
        if (!host->CreateScriptInstance(m_projectAssetId, m_typeName, context.Entity, m_instanceHandle, m_lastStatus))
        {
            Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
            return;
        }
    }

    const Pragma::Scripting::ManagedScriptTimeSnapshot time
    {
        context.Time.DeltaSeconds,
        context.Time.ElapsedSeconds,
        context.Time.FrameIndex
    };
    if (!host->StartScriptInstance(m_projectAssetId, m_instanceHandle, context.World, time, m_lastStatus))
    {
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
    }
}

void ManagedScriptComponent::OnUpdate(const BehaviourContext& context)
{
    if (m_instanceHandle == 0)
    {
        return;
    }

    auto* host = context.World.GetManagedScriptHost();
    if (host == nullptr)
    {
        m_lastStatus = "managed script host is unavailable.";
        return;
    }

    const Pragma::Scripting::ManagedScriptTimeSnapshot time
    {
        context.Time.DeltaSeconds,
        context.Time.ElapsedSeconds,
        context.Time.FrameIndex
    };
    if (!host->UpdateScriptInstance(m_projectAssetId, m_instanceHandle, context.World, time, m_lastStatus))
    {
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
    }
}

void ManagedScriptComponent::OnDestroy(const BehaviourContext& context)
{
    if (m_instanceHandle == 0)
    {
        return;
    }

    auto* host = context.World.GetManagedScriptHost();
    if (host == nullptr)
    {
        m_instanceHandle = 0;
        return;
    }

    const Pragma::Scripting::ManagedScriptTimeSnapshot time
    {
        context.Time.DeltaSeconds,
        context.Time.ElapsedSeconds,
        context.Time.FrameIndex
    };
    if (!host->DestroyScriptInstance(m_projectAssetId, m_instanceHandle, context.World, time, m_lastStatus))
    {
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
    }

    m_instanceHandle = 0;
}
}
