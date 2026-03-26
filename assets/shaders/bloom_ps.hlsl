cbuffer BloomConstants : register(b0)
{
    float Threshold;
    float Intensity;
    float Quality;
    float Padding0;
    float2 SourceTexelSize;
    float2 Padding1;
};

Texture2D SceneColorTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 SampleScene(float2 uv)
{
    return SceneColorTexture.Sample(LinearSampler, uv).rgb;
}

float3 ExtractBright(float3 color, float threshold)
{
    const float brightness = max(color.r, max(color.g, color.b));
    const float soft = saturate((brightness - threshold) / max(threshold, 0.001f));
    return color * soft;
}

float4 main(VSOutput input) : SV_TARGET
{
    const float2 texel = SourceTexelSize * 2.0f;
    const float3 centerBright = ExtractBright(SampleScene(input.texcoord), Threshold);
    float3 accum = centerBright;

    // Cheap balanced/performance path: keep bloom subtle and avoid extra taps unless the preset asks for them.
    if (Quality < 0.5f)
    {
        return float4(max(accum, 0.0f), 1.0f);
    }

    if (Quality >= 1.5f)
    {
        accum *= 0.28f;
        accum += ExtractBright(SampleScene(input.texcoord + float2(texel.x, 0.0f)), Threshold) * 0.18f;
        accum += ExtractBright(SampleScene(input.texcoord - float2(texel.x, 0.0f)), Threshold) * 0.18f;
        accum += ExtractBright(SampleScene(input.texcoord + float2(0.0f, texel.y)), Threshold) * 0.18f;
        accum += ExtractBright(SampleScene(input.texcoord - float2(0.0f, texel.y)), Threshold) * 0.18f;
    }
    else if (Quality >= 0.5f)
    {
        accum *= 0.46f;
        accum += ExtractBright(SampleScene(input.texcoord + float2(texel.x, 0.0f)), Threshold) * 0.27f;
        accum += ExtractBright(SampleScene(input.texcoord - float2(texel.x, 0.0f)), Threshold) * 0.27f;
    }

    return float4(max(accum, 0.0f), 1.0f);
}
