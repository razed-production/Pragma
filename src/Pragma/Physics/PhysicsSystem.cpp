#include "Pragma/Physics/PhysicsSystem.h"

#include "Pragma/Core/Assert.h"
#include "Pragma/Core/Log.h"
#include "Pragma/Core/Profiler.h"
#include "Pragma/Math/Quaternion.h"
#include "Pragma/Math/Vector3.h"
#include "Pragma/Physics/BoxColliderComponent.h"
#include "Pragma/Physics/RigidBodyComponent.h"
#include "Pragma/Renderer/Scene.h"
#include "Pragma/Renderer/Transform.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cmath>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>

namespace Pragma::Physics
{
namespace
{
constexpr JPH::ObjectLayer kStaticObjectLayer = 0;
constexpr JPH::ObjectLayer kDynamicObjectLayer = 1;
constexpr JPH::ObjectLayer kNoCollisionObjectLayer = 2;
constexpr JPH::uint kNumObjectLayers = 3;

constexpr JPH::BroadPhaseLayer kStaticBroadPhaseLayer(0);
constexpr JPH::BroadPhaseLayer kDynamicBroadPhaseLayer(1);
constexpr JPH::uint kNumBroadPhaseLayers = 2;

class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BroadPhaseLayerInterfaceImpl()
    {
        m_objectToBroadPhase[kStaticObjectLayer] = kStaticBroadPhaseLayer;
        m_objectToBroadPhase[kDynamicObjectLayer] = kDynamicBroadPhaseLayer;
        m_objectToBroadPhase[kNoCollisionObjectLayer] = kDynamicBroadPhaseLayer;
    }

    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override
    {
        return kNumBroadPhaseLayers;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer layer) const override
    {
        PRAGMA_ASSERT(layer < kNumObjectLayers, "Jolt broad phase received an invalid object layer.");
        return m_objectToBroadPhase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(const JPH::BroadPhaseLayer layer) const override
    {
        switch (layer.GetValue())
        {
        case 0: return "Static";
        case 1: return "Dynamic";
        default: return "Invalid";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer m_objectToBroadPhase[kNumObjectLayers]{};
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer objectLayer, const JPH::BroadPhaseLayer broadPhaseLayer) const override
    {
        switch (objectLayer)
        {
        case kStaticObjectLayer:
            return broadPhaseLayer == kDynamicBroadPhaseLayer;
        case kDynamicObjectLayer:
            return true;
        case kNoCollisionObjectLayer:
            return false;
        default:
            PRAGMA_ASSERT(false, "Jolt object-vs-broadphase filter received an invalid object layer.");
            return false;
        }
    }
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer objectLayer1, const JPH::ObjectLayer objectLayer2) const override
    {
        switch (objectLayer1)
        {
        case kStaticObjectLayer:
            return objectLayer2 == kDynamicObjectLayer;
        case kDynamicObjectLayer:
            return true;
        case kNoCollisionObjectLayer:
            return false;
        default:
            PRAGMA_ASSERT(false, "Jolt object layer pair filter received an invalid object layer.");
            return false;
        }
    }
};

void TraceImpl(const char* format, ...)
{
    char buffer[1024]{};
    va_list args;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, std::string("Jolt: ") + buffer);
}

#ifdef JPH_ENABLE_ASSERTS
bool AssertFailedImpl(const char* expression, const char* message, const char* file, JPH::uint line)
{
    Pragma::Core::Log(
        Pragma::Core::LogCategory::Scene,
        Pragma::Core::LogLevel::Error,
        std::string("Jolt assert at ") + file + ':' + std::to_string(line) + " (" + expression + ") " + (message != nullptr ? message : ""));
    return true;
}
#endif

[[nodiscard]] JPH::Quat ToJoltQuat(const Pragma::Renderer::Transform& transform) noexcept
{
    const Pragma::Math::Quaternion rotation = Pragma::Renderer::ToQuaternion(transform);
    return JPH::Quat(rotation.X, rotation.Y, rotation.Z, rotation.W);
}

[[nodiscard]] Pragma::Math::Vector3 ToPragmaVector3(const JPH::RVec3& value) noexcept
{
    return
    {
        static_cast<float>(value.GetX()),
        static_cast<float>(value.GetY()),
        static_cast<float>(value.GetZ())
    };
}

[[nodiscard]] Pragma::Math::Vector3 ToPragmaEuler(const JPH::Quat& value) noexcept
{
    return Pragma::Math::EulerRadiansFromQuaternion({ value.GetX(), value.GetY(), value.GetZ(), value.GetW() });
}

[[nodiscard]] JPH::Vec3 ToJoltHalfExtent(const Pragma::Math::Vector3& halfExtent, const Pragma::Math::Vector3& scale) noexcept
{
    const Pragma::Math::Vector3 scaledHalfExtent
    {
        std::max(0.01f, std::abs(halfExtent.X * scale.X)),
        std::max(0.01f, std::abs(halfExtent.Y * scale.Y)),
        std::max(0.01f, std::abs(halfExtent.Z * scale.Z))
    };

    return JPH::Vec3(scaledHalfExtent.X, scaledHalfExtent.Y, scaledHalfExtent.Z);
}

[[nodiscard]] JPH::EMotionType ToJoltMotionType(const Pragma::Renderer::RigidBodyMotionType motionType) noexcept
{
    switch (motionType)
    {
    case Pragma::Renderer::RigidBodyMotionType::Static:
        return JPH::EMotionType::Static;
    case Pragma::Renderer::RigidBodyMotionType::Kinematic:
        return JPH::EMotionType::Kinematic;
    case Pragma::Renderer::RigidBodyMotionType::Dynamic:
    default:
        return JPH::EMotionType::Dynamic;
    }
}

[[nodiscard]] JPH::ObjectLayer ToJoltObjectLayer(
    const Pragma::Renderer::RigidBodyMotionType motionType,
    const Pragma::Renderer::RigidBodyCollisionLayer collisionLayer) noexcept
{
    if (collisionLayer == Pragma::Renderer::RigidBodyCollisionLayer::NoCollision)
    {
        return kNoCollisionObjectLayer;
    }

    return motionType == Pragma::Renderer::RigidBodyMotionType::Static ? kStaticObjectLayer : kDynamicObjectLayer;
}
}

struct PhysicsSystem::Impl
{
    struct BodySignature
    {
        bool Enabled = true;
        Pragma::Renderer::RigidBodyMotionType MotionType = Pragma::Renderer::RigidBodyMotionType::Dynamic;
        Pragma::Renderer::RigidBodyCollisionLayer CollisionLayer = Pragma::Renderer::RigidBodyCollisionLayer::Default;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float LinearDamping = 0.05f;
        float AngularDamping = 0.05f;
        float GravityFactor = 1.0f;
        Pragma::Math::Vector3 HalfExtent{ 0.5f, 0.5f, 0.5f };
        Pragma::Math::Vector3 WorldScale{ 1.0f, 1.0f, 1.0f };

        [[nodiscard]] bool operator==(const BodySignature& other) const noexcept
        {
            return Enabled == other.Enabled &&
                MotionType == other.MotionType &&
                CollisionLayer == other.CollisionLayer &&
                Friction == other.Friction &&
                Restitution == other.Restitution &&
                LinearDamping == other.LinearDamping &&
                AngularDamping == other.AngularDamping &&
                GravityFactor == other.GravityFactor &&
                HalfExtent.X == other.HalfExtent.X &&
                HalfExtent.Y == other.HalfExtent.Y &&
                HalfExtent.Z == other.HalfExtent.Z &&
                WorldScale.X == other.WorldScale.X &&
                WorldScale.Y == other.WorldScale.Y &&
                WorldScale.Z == other.WorldScale.Z;
        }
    };

    struct BodyEntry
    {
        JPH::BodyID BodyId;
        BodySignature Signature;
    };

    BroadPhaseLayerInterfaceImpl BroadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl ObjectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl ObjectLayerPairFilter;
    std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> JobSystem;
    std::unique_ptr<JPH::PhysicsSystem> JoltSystem;
    std::unordered_map<Pragma::Renderer::EntityId, BodyEntry> Bodies;
    bool Initialized = false;
};

PhysicsSystem::PhysicsSystem()
    : m_impl(std::make_unique<Impl>())
{
}

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

void PhysicsSystem::Initialize()
{
    if (m_impl->Initialized)
    {
        return;
    }

    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = AssertFailedImpl;
#endif

    if (JPH::Factory::sInstance == nullptr)
    {
        JPH::Factory::sInstance = new JPH::Factory();
    }
    JPH::RegisterTypes();

    m_impl->TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    const unsigned int workerCount = std::max(1u, std::thread::hardware_concurrency());
    m_impl->JobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        std::max(1u, workerCount - 1u));
    m_impl->JoltSystem = std::make_unique<JPH::PhysicsSystem>();
    m_impl->JoltSystem->Init(
        2048,
        0,
        2048,
        2048,
        m_impl->BroadPhaseLayerInterface,
        m_impl->ObjectVsBroadPhaseLayerFilter,
        m_impl->ObjectLayerPairFilter);
    m_impl->Initialized = true;

    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, "Jolt Physics initialized.");
}

void PhysicsSystem::Shutdown()
{
    if (!m_impl->Initialized)
    {
        return;
    }

    if (m_impl->JoltSystem != nullptr)
    {
        JPH::BodyInterface& bodyInterface = m_impl->JoltSystem->GetBodyInterface();
        for (const auto& [entityId, entry] : m_impl->Bodies)
        {
            (void)entityId;
            bodyInterface.RemoveBody(entry.BodyId);
            bodyInterface.DestroyBody(entry.BodyId);
        }
    }

    m_impl->Bodies.clear();
    m_impl->JoltSystem.reset();
    m_impl->JobSystem.reset();
    m_impl->TempAllocator.reset();
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    m_impl->Initialized = false;
    Pragma::Core::Log(Pragma::Core::LogCategory::Scene, Pragma::Core::LogLevel::Info, "Jolt Physics shut down.");
}

void PhysicsSystem::Update(Pragma::Renderer::Scene& scene, const Pragma::Core::EngineTime& time)
{
    if (!m_impl->Initialized)
    {
        return;
    }

    PRAGMA_PROFILE_SCOPE("Physics Sync");

    JPH::BodyInterface& bodyInterface = m_impl->JoltSystem->GetBodyInterface();
    std::unordered_map<Pragma::Renderer::EntityId, bool> seenBodies;
    seenBodies.reserve(scene.GetObjects().size());

    for (const Pragma::Renderer::SceneObject& object : scene.GetObjects())
    {
        const auto* rigidBody = object.GetRigidBody();
        const auto* boxCollider = object.GetBoxCollider();
        const bool shouldHaveBody = rigidBody != nullptr && boxCollider != nullptr && rigidBody->Enabled;

        if (!shouldHaveBody)
        {
            const auto bodyIt = m_impl->Bodies.find(object.Id);
            if (bodyIt != m_impl->Bodies.end())
            {
                bodyInterface.RemoveBody(bodyIt->second.BodyId);
                bodyInterface.DestroyBody(bodyIt->second.BodyId);
                m_impl->Bodies.erase(bodyIt);
            }
            continue;
        }

        const Pragma::Renderer::Transform worldTransform = scene.GetWorldTransform(object.Id);
        Impl::BodySignature signature;
        signature.Enabled = rigidBody->Enabled;
        signature.MotionType = rigidBody->MotionType;
        signature.CollisionLayer = rigidBody->CollisionLayer;
        signature.Friction = rigidBody->Friction;
        signature.Restitution = rigidBody->Restitution;
        signature.LinearDamping = rigidBody->LinearDamping;
        signature.AngularDamping = rigidBody->AngularDamping;
        signature.GravityFactor = rigidBody->GravityFactor;
        signature.HalfExtent = boxCollider->HalfExtent;
        signature.WorldScale = worldTransform.Scale;

        auto bodyIt = m_impl->Bodies.find(object.Id);
        if (bodyIt != m_impl->Bodies.end() && !(bodyIt->second.Signature == signature))
        {
            bodyInterface.RemoveBody(bodyIt->second.BodyId);
            bodyInterface.DestroyBody(bodyIt->second.BodyId);
            m_impl->Bodies.erase(bodyIt);
            bodyIt = m_impl->Bodies.end();
        }

        if (bodyIt == m_impl->Bodies.end())
        {
            JPH::ShapeRefC shape = new JPH::BoxShape(ToJoltHalfExtent(boxCollider->HalfExtent, worldTransform.Scale));
            JPH::BodyCreationSettings settings(
                shape,
                JPH::RVec3(worldTransform.Position.X, worldTransform.Position.Y, worldTransform.Position.Z),
                ToJoltQuat(worldTransform),
                ToJoltMotionType(rigidBody->MotionType),
                ToJoltObjectLayer(rigidBody->MotionType, rigidBody->CollisionLayer));
            settings.mFriction = rigidBody->Friction;
            settings.mRestitution = rigidBody->Restitution;
            settings.mLinearDamping = rigidBody->LinearDamping;
            settings.mAngularDamping = rigidBody->AngularDamping;
            settings.mGravityFactor = rigidBody->GravityFactor;

            const JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(
                settings,
                rigidBody->MotionType == Pragma::Renderer::RigidBodyMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
            m_impl->Bodies.emplace(object.Id, Impl::BodyEntry{ bodyId, signature });
            bodyIt = m_impl->Bodies.find(object.Id);
        }

        seenBodies.emplace(object.Id, true);

        if (rigidBody->MotionType == Pragma::Renderer::RigidBodyMotionType::Static)
        {
            bodyInterface.SetPositionAndRotation(
                bodyIt->second.BodyId,
                JPH::RVec3(worldTransform.Position.X, worldTransform.Position.Y, worldTransform.Position.Z),
                ToJoltQuat(worldTransform),
                JPH::EActivation::DontActivate);
        }
        else if (rigidBody->MotionType == Pragma::Renderer::RigidBodyMotionType::Kinematic)
        {
            bodyInterface.MoveKinematic(
                bodyIt->second.BodyId,
                JPH::RVec3(worldTransform.Position.X, worldTransform.Position.Y, worldTransform.Position.Z),
                ToJoltQuat(worldTransform),
                std::max(time.DeltaSeconds, 1.0e-4f));
        }
    }

    for (auto it = m_impl->Bodies.begin(); it != m_impl->Bodies.end();)
    {
        if (!scene.IsEntityAlive(it->first) || !seenBodies.contains(it->first))
        {
            bodyInterface.RemoveBody(it->second.BodyId);
            bodyInterface.DestroyBody(it->second.BodyId);
            it = m_impl->Bodies.erase(it);
        }
        else
        {
            ++it;
        }
    }

    {
        PRAGMA_PROFILE_SCOPE("Physics Step");
        m_impl->JoltSystem->Update(std::max(time.DeltaSeconds, 1.0f / 240.0f), 1, m_impl->TempAllocator.get(), m_impl->JobSystem.get());
    }

    {
        PRAGMA_PROFILE_SCOPE("Physics Writeback");
        for (const Pragma::Renderer::SceneObject& object : scene.GetObjects())
        {
            const auto* rigidBody = object.GetRigidBody();
            if (rigidBody == nullptr || rigidBody->MotionType != Pragma::Renderer::RigidBodyMotionType::Dynamic)
            {
                continue;
            }

            const auto bodyIt = m_impl->Bodies.find(object.Id);
            if (bodyIt == m_impl->Bodies.end())
            {
                continue;
            }

            const JPH::RVec3 position = bodyInterface.GetCenterOfMassPosition(bodyIt->second.BodyId);
            const JPH::Quat rotation = bodyInterface.GetRotation(bodyIt->second.BodyId);

            Pragma::Renderer::Transform worldTransform = scene.GetWorldTransform(object.Id);
            worldTransform.Position = ToPragmaVector3(position);
            worldTransform.RotationRadians = ToPragmaEuler(rotation);
            const bool updated = scene.SetWorldTransform(object.Id, worldTransform);
            PRAGMA_ASSERT(updated, "PhysicsSystem failed to write a dynamic body transform back into the scene.");
        }
    }
}

PhysicsSystem::BodyDebugState PhysicsSystem::GetBodyDebugState(const Pragma::Renderer::EntityId entityId)
{
    BodyDebugState state;
    if (!m_impl->Initialized)
    {
        return state;
    }

    const auto bodyIt = m_impl->Bodies.find(entityId);
    if (bodyIt == m_impl->Bodies.end())
    {
        return state;
    }

    state.HasBody = true;
    state.IsActive = m_impl->JoltSystem->GetBodyInterface().IsActive(bodyIt->second.BodyId);
    return state;
}

std::size_t PhysicsSystem::GetBodyCount() const noexcept
{
    return m_impl->Bodies.size();
}

std::size_t PhysicsSystem::GetActiveBodyCount()
{
    if (!m_impl->Initialized)
    {
        return 0;
    }

    std::size_t activeBodyCount = 0;
    JPH::BodyInterface& bodyInterface = m_impl->JoltSystem->GetBodyInterface();
    for (const auto& [entityId, entry] : m_impl->Bodies)
    {
        (void)entityId;
        if (bodyInterface.IsActive(entry.BodyId))
        {
            ++activeBodyCount;
        }
    }

    return activeBodyCount;
}
}
