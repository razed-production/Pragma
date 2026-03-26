struct PSInput
{
    float4 position : SV_POSITION;
    float depth : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    const float depth = saturate(input.depth);
    return float4(depth, depth, depth, 1.0f);
}
