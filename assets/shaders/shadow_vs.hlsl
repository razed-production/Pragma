#define MAX_BATCHED_INSTANCES 64

cbuffer ShadowInstanceConstants : register(b0)
{
    row_major float4x4 InstanceWorldLightClip[MAX_BATCHED_INSTANCES];
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
    float depth : TEXCOORD0;
};

PSInput main(VSInput input, uint instanceId : SV_InstanceID)
{
    const uint instanceIndex = min(instanceId, MAX_BATCHED_INSTANCES - 1);
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), InstanceWorldLightClip[instanceIndex]);
    output.depth = output.position.z / output.position.w;
    return output;
}
