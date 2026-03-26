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

ManagedScriptComponent::LifecycleState ManagedScriptComponent::GetLifecycleState() const noexcept
{
    return m_lifecycleState;
}

std::uint64_t ManagedScriptComponent::GetUpdateCount() const noexcept
{
    return m_updateCount;
}

std::uint64_t ManagedScriptComponent::GetLastUpdatedFrame() const noexcept
{
    return m_lastUpdatedFrame;
}

bool ManagedScriptComponent::WasLastCallSuccessful() const noexcept
{
    return m_lastCallSucceeded;
}

void ManagedScriptComponent::SetBinding(Pragma::Assets::AssetId projectAssetId, std::string typeName)
{
    m_projectAssetId = std::move(projectAssetId);
    m_typeName = std::move(typeName);
    m_instanceHandle = 0;
    m_lastStatus.clear();
    m_lifecycleState = LifecycleState::Uninitialized;
    m_updateCount = 0;
    m_lastUpdatedFrame = 0;
    m_lastCallSucceeded = false;
}

void ManagedScriptComponent::OnStart(const BehaviourContext& context)
{
    auto* host = context.World.GetManagedScriptHost();
    if (host == nullptr)
    {
        m_lastStatus = "managed script host is unavailable.";
        m_lifecycleState = LifecycleState::Failed;
        m_lastCallSucceeded = false;
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
        return;
    }

    if (m_instanceHandle == 0)
    {
        if (!host->CreateScriptInstance(m_projectAssetId, m_typeName, context.Entity, m_instanceHandle, m_lastStatus))
        {
            m_lifecycleState = LifecycleState::Failed;
            m_lastCallSucceeded = false;
            Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
            return;
        }

        m_lifecycleState = LifecycleState::Created;
        m_lastCallSucceeded = true;
    }

    const Pragma::Scripting::ManagedScriptTimeSnapshot time
    {
        context.Time.DeltaSeconds,
        context.Time.ElapsedSeconds,
        context.Time.FrameIndex,
        &context.Input
    };
    if (!host->StartScriptInstance(m_projectAssetId, m_instanceHandle, context.World, time, m_lastStatus))
    {
        m_lifecycleState = LifecycleState::Failed;
        m_lastCallSucceeded = false;
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
        return;
    }

    m_lifecycleState = LifecycleState::Started;
    m_lastCallSucceeded = true;
    m_lastUpdatedFrame = context.Time.FrameIndex;
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
        m_lifecycleState = LifecycleState::Failed;
        m_lastCallSucceeded = false;
        return;
    }

    const Pragma::Scripting::ManagedScriptTimeSnapshot time
    {
        context.Time.DeltaSeconds,
        context.Time.ElapsedSeconds,
        context.Time.FrameIndex,
        &context.Input
    };
    if (!host->UpdateScriptInstance(m_projectAssetId, m_instanceHandle, context.World, time, m_lastStatus))
    {
        m_lifecycleState = LifecycleState::Failed;
        m_lastCallSucceeded = false;
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
        return;
    }

    m_lifecycleState = LifecycleState::Running;
    m_lastCallSucceeded = true;
    ++m_updateCount;
    m_lastUpdatedFrame = context.Time.FrameIndex;
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
        m_lifecycleState = LifecycleState::Destroyed;
        m_lastCallSucceeded = false;
        m_instanceHandle = 0;
        return;
    }

    const Pragma::Scripting::ManagedScriptTimeSnapshot time
    {
        context.Time.DeltaSeconds,
        context.Time.ElapsedSeconds,
        context.Time.FrameIndex,
        &context.Input
    };
    if (!host->DestroyScriptInstance(m_projectAssetId, m_instanceHandle, context.World, time, m_lastStatus))
    {
        m_lifecycleState = LifecycleState::Failed;
        m_lastCallSucceeded = false;
        Pragma::Core::Log(Pragma::Core::LogCategory::General, Pragma::Core::LogLevel::Warning, m_lastStatus);
    }
    else
    {
        m_lifecycleState = LifecycleState::Destroyed;
        m_lastCallSucceeded = true;
    }

    m_instanceHandle = 0;
}
}
