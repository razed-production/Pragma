using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Pragma.Managed
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedTimeSnapshot
    {
        public float DeltaSeconds;
        public float ElapsedSeconds;
        public ulong FrameIndex;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedVector3
    {
        public float X;
        public float Y;
        public float Z;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedTransformSnapshot
    {
        public ManagedVector3 Position;
        public ManagedVector3 RotationRadians;
        public ManagedVector3 Scale;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedCameraSnapshot
    {
        public float PitchRadians;
        public float FieldOfViewRadians;
        public float NearPlane;
        public float FarPlane;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedLightSnapshot
    {
        public ManagedVector3 Direction;
        public float Intensity;
        public ManagedVector3 Color;
        public float Padding0;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedRigidBodySnapshot
    {
        public int Enabled;
        public int MotionType;
        public int CollisionLayer;
        public float Friction;
        public float Restitution;
        public float LinearDamping;
        public float AngularDamping;
        public float GravityFactor;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedInputSnapshot
    {
        public int MoveForward;
        public int MoveBackward;
        public int MoveLeft;
        public int MoveRight;
        public int MoveUp;
        public int MoveDown;
        public int LookLeft;
        public int LookRight;
        public int LookUp;
        public int LookDown;
        public int FastMove;
        public int RightMouseButtonDown;
        public int MouseDeltaX;
        public int MouseDeltaY;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ManagedBindings
    {
        public IntPtr LogCallback;
        public IntPtr FindEntityByName;
        public IntPtr IsEntityValid;
        public IntPtr GetEntityName;
        public IntPtr GetParent;
        public IntPtr GetChildCount;
        public IntPtr GetChildAt;
        public IntPtr GetActiveCameraEntity;
        public IntPtr GetEntityCount;
        public IntPtr GetTransform;
        public IntPtr SetTransform;
        public IntPtr GetCamera;
        public IntPtr SetCamera;
        public IntPtr GetLight;
        public IntPtr SetLight;
        public IntPtr GetRigidBody;
        public IntPtr SetRigidBody;
        public IntPtr GetInputSnapshot;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogCallback(IntPtr message);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong FindEntityByNameCallback(IntPtr sceneContext, IntPtr name);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int IsEntityValidCallback(IntPtr sceneContext, ulong entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetEntityNameCallback(IntPtr sceneContext, ulong entityId, IntPtr buffer, int bufferSize);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong GetParentCallback(IntPtr sceneContext, ulong entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong GetChildCountCallback(IntPtr sceneContext, ulong entityId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong GetChildAtCallback(IntPtr sceneContext, ulong entityId, ulong childIndex);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong GetActiveCameraEntityCallback(IntPtr sceneContext);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong GetEntityCountCallback(IntPtr sceneContext);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetTransformCallback(IntPtr sceneContext, ulong entityId, out ManagedTransformSnapshot transform);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int SetTransformCallback(IntPtr sceneContext, ulong entityId, ref ManagedTransformSnapshot transform);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetCameraCallback(IntPtr sceneContext, ulong entityId, out ManagedCameraSnapshot camera);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int SetCameraCallback(IntPtr sceneContext, ulong entityId, ref ManagedCameraSnapshot camera);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetLightCallback(IntPtr sceneContext, ulong entityId, out ManagedLightSnapshot light);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int SetLightCallback(IntPtr sceneContext, ulong entityId, ref ManagedLightSnapshot light);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetRigidBodyCallback(IntPtr sceneContext, ulong entityId, out ManagedRigidBodySnapshot rigidBody);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int SetRigidBodyCallback(IntPtr sceneContext, ulong entityId, ref ManagedRigidBodySnapshot rigidBody);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GetInputSnapshotCallback(IntPtr sceneContext, out ManagedInputSnapshot input);

    internal sealed class ManagedApiContext
    {
        private readonly IntPtr _sceneContext;
        private readonly LogCallback _log;
        private readonly FindEntityByNameCallback _findEntityByName;
        private readonly IsEntityValidCallback _isEntityValid;
        private readonly GetEntityNameCallback _getEntityName;
        private readonly GetParentCallback _getParent;
        private readonly GetChildCountCallback _getChildCount;
        private readonly GetChildAtCallback _getChildAt;
        private readonly GetActiveCameraEntityCallback _getActiveCameraEntity;
        private readonly GetEntityCountCallback _getEntityCount;
        private readonly GetTransformCallback _getTransform;
        private readonly SetTransformCallback _setTransform;
        private readonly GetCameraCallback _getCamera;
        private readonly SetCameraCallback _setCamera;
        private readonly GetLightCallback _getLight;
        private readonly SetLightCallback _setLight;
        private readonly GetRigidBodyCallback _getRigidBody;
        private readonly SetRigidBodyCallback _setRigidBody;
        private readonly GetInputSnapshotCallback _getInputSnapshot;
        private readonly ManagedTimeSnapshot _time;

        public ManagedApiContext(ManagedTimeSnapshot time, ManagedBindings bindings, IntPtr sceneContext)
        {
            _time = time;
            _sceneContext = sceneContext;
            _log = (LogCallback)Marshal.GetDelegateForFunctionPointer(bindings.LogCallback, typeof(LogCallback));
            _findEntityByName =
                (FindEntityByNameCallback)Marshal.GetDelegateForFunctionPointer(bindings.FindEntityByName, typeof(FindEntityByNameCallback));
            _isEntityValid =
                (IsEntityValidCallback)Marshal.GetDelegateForFunctionPointer(bindings.IsEntityValid, typeof(IsEntityValidCallback));
            _getEntityName =
                (GetEntityNameCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetEntityName, typeof(GetEntityNameCallback));
            _getParent =
                (GetParentCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetParent, typeof(GetParentCallback));
            _getChildCount =
                (GetChildCountCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetChildCount, typeof(GetChildCountCallback));
            _getChildAt =
                (GetChildAtCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetChildAt, typeof(GetChildAtCallback));
            _getActiveCameraEntity =
                (GetActiveCameraEntityCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetActiveCameraEntity, typeof(GetActiveCameraEntityCallback));
            _getEntityCount =
                (GetEntityCountCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetEntityCount, typeof(GetEntityCountCallback));
            _getTransform =
                (GetTransformCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetTransform, typeof(GetTransformCallback));
            _setTransform =
                (SetTransformCallback)Marshal.GetDelegateForFunctionPointer(bindings.SetTransform, typeof(SetTransformCallback));
            _getCamera =
                (GetCameraCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetCamera, typeof(GetCameraCallback));
            _setCamera =
                (SetCameraCallback)Marshal.GetDelegateForFunctionPointer(bindings.SetCamera, typeof(SetCameraCallback));
            _getLight =
                (GetLightCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetLight, typeof(GetLightCallback));
            _setLight =
                (SetLightCallback)Marshal.GetDelegateForFunctionPointer(bindings.SetLight, typeof(SetLightCallback));
            _getRigidBody =
                (GetRigidBodyCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetRigidBody, typeof(GetRigidBodyCallback));
            _setRigidBody =
                (SetRigidBodyCallback)Marshal.GetDelegateForFunctionPointer(bindings.SetRigidBody, typeof(SetRigidBodyCallback));
            _getInputSnapshot =
                (GetInputSnapshotCallback)Marshal.GetDelegateForFunctionPointer(bindings.GetInputSnapshot, typeof(GetInputSnapshotCallback));
        }

        public ManagedTimeSnapshot Time
        {
            get { return _time; }
        }

        public void Log(string message)
        {
            IntPtr messagePtr = Marshal.StringToHGlobalAnsi(message);
            try
            {
                _log(messagePtr);
            }
            finally
            {
                Marshal.FreeHGlobal(messagePtr);
            }
        }

        public ulong FindEntityByName(string name)
        {
            IntPtr namePtr = Marshal.StringToHGlobalAnsi(name);
            try
            {
                return _findEntityByName(_sceneContext, namePtr);
            }
            finally
            {
                Marshal.FreeHGlobal(namePtr);
            }
        }

        public bool IsEntityValid(ulong entityId)
        {
            return _isEntityValid(_sceneContext, entityId) != 0;
        }

        public string GetEntityName(ulong entityId)
        {
            const int BufferSize = 256;
            IntPtr buffer = Marshal.AllocHGlobal(BufferSize);
            try
            {
                for (int i = 0; i < BufferSize; ++i)
                {
                    Marshal.WriteByte(buffer, i, 0);
                }

                return _getEntityName(_sceneContext, entityId, buffer, BufferSize) != 0
                    ? Marshal.PtrToStringAnsi(buffer) ?? string.Empty
                    : string.Empty;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }

        public ulong GetParent(ulong entityId)
        {
            return _getParent(_sceneContext, entityId);
        }

        public ulong GetChildCount(ulong entityId)
        {
            return _getChildCount(_sceneContext, entityId);
        }

        public ulong GetChildAt(ulong entityId, ulong childIndex)
        {
            return _getChildAt(_sceneContext, entityId, childIndex);
        }

        public ulong GetActiveCameraEntity()
        {
            return _getActiveCameraEntity(_sceneContext);
        }

        public ulong GetEntityCount()
        {
            return _getEntityCount(_sceneContext);
        }

        public bool TryGetTransform(ulong entityId, out ManagedTransformSnapshot transform)
        {
            return _getTransform(_sceneContext, entityId, out transform) != 0;
        }

        public bool SetTransform(ulong entityId, ref ManagedTransformSnapshot transform)
        {
            return _setTransform(_sceneContext, entityId, ref transform) != 0;
        }

        public bool TryGetCamera(ulong entityId, out ManagedCameraSnapshot camera)
        {
            return _getCamera(_sceneContext, entityId, out camera) != 0;
        }

        public bool SetCamera(ulong entityId, ref ManagedCameraSnapshot camera)
        {
            return _setCamera(_sceneContext, entityId, ref camera) != 0;
        }

        public bool TryGetLight(ulong entityId, out ManagedLightSnapshot light)
        {
            return _getLight(_sceneContext, entityId, out light) != 0;
        }

        public bool SetLight(ulong entityId, ref ManagedLightSnapshot light)
        {
            return _setLight(_sceneContext, entityId, ref light) != 0;
        }

        public bool TryGetRigidBody(ulong entityId, out ManagedRigidBodySnapshot rigidBody)
        {
            return _getRigidBody(_sceneContext, entityId, out rigidBody) != 0;
        }

        public bool SetRigidBody(ulong entityId, ref ManagedRigidBodySnapshot rigidBody)
        {
            return _setRigidBody(_sceneContext, entityId, ref rigidBody) != 0;
        }

        public bool TryGetInputSnapshot(out ManagedInputSnapshot input)
        {
            return _getInputSnapshot(_sceneContext, out input) != 0;
        }
    }

    internal abstract class ManagedScriptBase
    {
        private readonly ulong _entityId;

        protected ManagedScriptBase(ulong entityId)
        {
            _entityId = entityId;
        }

        public ulong EntityId
        {
            get { return _entityId; }
        }

        public virtual void OnStart(ManagedApiContext context) { }
        public virtual void OnUpdate(ManagedApiContext context) { }
        public virtual void OnDestroy(ManagedApiContext context) { }
    }
}

namespace Pragma.Managed.Scripts
{
    internal sealed class FloatUpManagedScript : Pragma.Managed.ManagedScriptBase
    {
        private bool _hasBaseTransform;
        private bool _loggedFirstUpdate;
        private Pragma.Managed.ManagedTransformSnapshot _baseTransform;

        public FloatUpManagedScript(ulong entityId)
            : base(entityId)
        {
        }

        public override void OnStart(Pragma.Managed.ManagedApiContext context)
        {
            string entityName = context.GetEntityName(EntityId);
            ulong parentId = context.GetParent(EntityId);
            ulong childCount = context.GetChildCount(EntityId);
            ulong activeCameraId = context.GetActiveCameraEntity();
            ulong entityCount = context.GetEntityCount();
            bool isValid = context.IsEntityValid(EntityId);
            ManagedCameraSnapshot activeCamera = new ManagedCameraSnapshot();
            bool hasActiveCamera = activeCameraId != 0 && context.TryGetCamera(activeCameraId, out activeCamera);
            ulong lightEntityId = context.FindEntityByName("Sun Light");
            ManagedLightSnapshot sunLight = new ManagedLightSnapshot();
            bool hasSunLight = lightEntityId != 0 && context.TryGetLight(lightEntityId, out sunLight);
            ulong physicsCubeId = context.FindEntityByName("Physics Cube A");
            ManagedRigidBodySnapshot physicsBody = new ManagedRigidBodySnapshot();
            bool hasPhysicsBody = physicsCubeId != 0 && context.TryGetRigidBody(physicsCubeId, out physicsBody);
            ManagedInputSnapshot input = new ManagedInputSnapshot();
            bool hasInput = context.TryGetInputSnapshot(out input);

            if (context.TryGetTransform(EntityId, out _baseTransform))
            {
                _hasBaseTransform = true;
                context.Log(
                    string.Format(
                        "Managed script OnStart: entity={0}, name='{1}', valid={2}, parent={3}, children={4}, activeCamera={5}, cameraFov={6:F3}, light={7}, lightIntensity={8:F3}, rigidBody={9}, motion={10}, gravity={11:F3}, input={12}, forward={13}, fast={14}, entities={15}, baseY={16:F3}",
                        EntityId,
                        entityName,
                        isValid ? "yes" : "no",
                        parentId,
                        childCount,
                        activeCameraId,
                        hasActiveCamera ? activeCamera.FieldOfViewRadians : -1.0f,
                        lightEntityId,
                        hasSunLight ? sunLight.Intensity : -1.0f,
                        physicsCubeId,
                        hasPhysicsBody ? physicsBody.MotionType : -1,
                        hasPhysicsBody ? physicsBody.GravityFactor : -1.0f,
                        hasInput ? "yes" : "no",
                        hasInput && input.MoveForward != 0 ? "yes" : "no",
                        hasInput && input.FastMove != 0 ? "yes" : "no",
                        entityCount,
                        _baseTransform.Position.Y));
            }
            else
            {
                context.Log(
                    string.Format(
                        "Managed script OnStart failed to read transform for entity={0}, name='{1}', valid={2}, parent={3}, children={4}, activeCamera={5}, light={6}",
                        EntityId,
                        entityName,
                        isValid ? "yes" : "no",
                        parentId,
                        childCount,
                        activeCameraId,
                        lightEntityId));
            }
        }

        public override void OnUpdate(Pragma.Managed.ManagedApiContext context)
        {
            if (!_hasBaseTransform)
            {
                return;
            }

            Pragma.Managed.ManagedTransformSnapshot transform = _baseTransform;
            transform.Position.Y = _baseTransform.Position.Y + (float)Math.Sin(context.Time.ElapsedSeconds * 2.0f) * 0.25f;
            context.SetTransform(EntityId, ref transform);

            if (!_loggedFirstUpdate)
            {
                _loggedFirstUpdate = true;
                context.Log(string.Format("Managed script OnUpdate: entity={0}, y={1:F3}, frame={2}", EntityId, transform.Position.Y, context.Time.FrameIndex));
            }
        }

        public override void OnDestroy(Pragma.Managed.ManagedApiContext context)
        {
            if (_hasBaseTransform)
            {
                Pragma.Managed.ManagedTransformSnapshot transform = _baseTransform;
                context.SetTransform(EntityId, ref transform);
            }

            context.Log(string.Format("Managed script OnDestroy: entity={0}", EntityId));
        }
    }
}

namespace Pragma.Managed
{
    public static class Bootstrap
    {
        private static readonly Dictionary<int, ManagedScriptBase> Instances = new Dictionary<int, ManagedScriptBase>();
        private static int s_nextInstanceId = 1;

        private static ManagedScriptBase CreateScript(string typeName, ulong entityId)
        {
            switch (typeName)
            {
                case "Pragma.Managed.Scripts.FloatUpManagedScript":
                    return new Scripts.FloatUpManagedScript(entityId);
                default:
                    return null;
            }
        }

        [UnmanagedCallersOnly(EntryPoint = "CreateManagedScriptInstance")]
        public static int CreateManagedScriptInstance(IntPtr typeName, ulong entityId)
        {
            string managedTypeName = Marshal.PtrToStringAnsi(typeName);
            if (string.IsNullOrEmpty(managedTypeName))
            {
                return 0;
            }

            ManagedScriptBase script = CreateScript(managedTypeName, entityId);
            if (script == null)
            {
                return 0;
            }

            int instanceId = s_nextInstanceId++;
            Instances[instanceId] = script;
            return instanceId;
        }

        [UnmanagedCallersOnly(EntryPoint = "StartManagedScriptInstance")]
        public static int StartManagedScriptInstance(int instanceId, ManagedTimeSnapshot time, ManagedBindings bindings, IntPtr sceneContext)
        {
            ManagedScriptBase script;
            if (!Instances.TryGetValue(instanceId, out script))
            {
                return 0;
            }

            script.OnStart(new ManagedApiContext(time, bindings, sceneContext));
            return 1;
        }

        [UnmanagedCallersOnly(EntryPoint = "UpdateManagedScriptInstance")]
        public static int UpdateManagedScriptInstance(int instanceId, ManagedTimeSnapshot time, ManagedBindings bindings, IntPtr sceneContext)
        {
            ManagedScriptBase script;
            if (!Instances.TryGetValue(instanceId, out script))
            {
                return 0;
            }

            script.OnUpdate(new ManagedApiContext(time, bindings, sceneContext));
            return 1;
        }

        [UnmanagedCallersOnly(EntryPoint = "DestroyManagedScriptInstance")]
        public static int DestroyManagedScriptInstance(int instanceId, ManagedTimeSnapshot time, ManagedBindings bindings, IntPtr sceneContext)
        {
            ManagedScriptBase script;
            if (!Instances.TryGetValue(instanceId, out script))
            {
                return 0;
            }

            script.OnDestroy(new ManagedApiContext(time, bindings, sceneContext));
            Instances.Remove(instanceId);
            return 1;
        }

        [UnmanagedCallersOnly(EntryPoint = "RunManagedBindingProbe")]
        public static int RunManagedBindingProbe(ManagedTimeSnapshot time, ManagedBindings bindings, IntPtr sceneContext)
        {
            var context = new ManagedApiContext(time, bindings, sceneContext);
            ulong entityId = context.FindEntityByName("Center Cube");
            if (entityId == 0)
            {
                context.Log("Managed binding probe: entity 'Center Cube' was not found.");
                return -1;
            }

            ManagedTransformSnapshot originalTransform;
            if (!context.TryGetTransform(entityId, out originalTransform))
            {
                context.Log("Managed binding probe: failed to read transform for 'Center Cube'.");
                return -2;
            }

            ManagedTransformSnapshot movedTransform = originalTransform;
            movedTransform.Position.Y += 0.125f;
            if (!context.SetTransform(entityId, ref movedTransform))
            {
                context.Log("Managed binding probe: failed to write transform for 'Center Cube'.");
                return -3;
            }

            ManagedTransformSnapshot observedTransform;
            if (!context.TryGetTransform(entityId, out observedTransform))
            {
                context.Log("Managed binding probe: failed to re-read transform after write.");
                return -4;
            }

            context.SetTransform(entityId, ref originalTransform);
            context.Log(
                string.Format(
                    "Managed binding probe: entity={0}, beforeY={1:F3}, movedY={2:F3}, frame={3}",
                    entityId,
                    originalTransform.Position.Y,
                    observedTransform.Position.Y,
                    time.FrameIndex));
            return 2027;
        }
    }
}
