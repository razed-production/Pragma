#define MAX_BATCHED_INSTANCES 64

cbuffer GlobalFrameConstants : register(b0)
{
    float3 CameraPosition;
    float Exposure;
    float3 LightDirection;
    float LightIntensity;
    float3 LightColor;
    float AmbientStrength;
    float3 SkyZenithColor;
    float EnvironmentDiffuseStrength;
    float3 SkyHorizonColor;
    float EnvironmentSpecularStrength;
    float3 SkyGroundColor;
    float SkyAtmosphereDensity;
    float3 FogColor;
    float FogStartDistance;
    float FogDensity;
    float FogHeightFalloff;
    float FogMaxOpacity;
    float Padding0;
    float2 ShadowMapTexelSize;
    float ShadowBias;
    float ShadowStrength;
};

cbuffer InstanceConstants : register(b2)
{
    row_major float4x4 InstanceWorldViewProjection[MAX_BATCHED_INSTANCES];
    row_major float4x4 InstanceWorld[MAX_BATCHED_INSTANCES];
    row_major float4x4 InstanceWorldNoScale[MAX_BATCHED_INSTANCES];
    row_major float4x4 InstanceWorldLightClip[MAX_BATCHED_INSTANCES];
};

cbuffer MaterialConstants : register(b1)
{
    float4 BaseColor;
    float3 EmissiveColor;
    float Roughness;
    float Metallic;
    float AmbientOcclusion;
    float UseAlbedoTexture;
    float EmissiveIntensity;
    float UseNormalTexture;
    float UseOrmTexture;
    float UseEmissiveTexture;
    float NormalStrength;
};

struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPosition : TEXCOORD1;
    float4 shadowPosition : TEXCOORD2;
};

PSInput main(VSInput input, uint instanceId : SV_InstanceID)
{
    const uint instanceIndex = min(instanceId, MAX_BATCHED_INSTANCES - 1);
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), InstanceWorldViewProjection[instanceIndex]);
    output.color = input.color;
    output.normal = mul(float4(input.normal, 0.0f), InstanceWorldNoScale[instanceIndex]).xyz;
    output.texcoord = input.texcoord;
    output.worldPosition = mul(float4(input.position, 1.0f), InstanceWorld[instanceIndex]).xyz;
    output.shadowPosition = mul(float4(input.position, 1.0f), InstanceWorldLightClip[instanceIndex]);
    return output;
}
