#pragma once

#include "Pragma/Assets/AssetId.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Renderer/BehaviourComponent.h"

#include <string>

namespace Pragma::Renderer
{
class ManagedScriptComponent final : public BehaviourComponent
{
public:
    static constexpr ComponentType kType = ComponentType::ManagedScript;

    ManagedScriptComponent() = default;
    ManagedScriptComponent(Pragma::Assets::AssetId projectAssetId, std::string typeName);

    [[nodiscard]] ComponentType GetComponentType() const noexcept override;

    [[nodiscard]] const char* GetComponentName() const noexcept override;

    [[nodiscard]] const Pragma::Assets::AssetId& GetProjectAssetId() const noexcept;

    [[nodiscard]] const std::string& GetTypeName() const noexcept;

    [[nodiscard]] int GetInstanceHandle() const noexcept;

    [[nodiscard]] const std::string& GetLastStatus() const noexcept;

    void SetBinding(Pragma::Assets::AssetId projectAssetId, std::string typeName);
    void OnStart(const BehaviourContext& context) override;
    void OnUpdate(const BehaviourContext& context) override;
    void OnDestroy(const BehaviourContext& context) override;

private:
    Pragma::Assets::AssetId m_projectAssetId;
    std::string m_typeName;
    int m_instanceHandle = 0;
    std::string m_lastStatus;
};
}
