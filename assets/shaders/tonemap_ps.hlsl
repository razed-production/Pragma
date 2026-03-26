cbuffer TonemapConstants : register(b0)
{
    float Exposure;
    float BloomIntensity;
    float FxaaEnabled;
    float FxaaSubpixel;
    float FxaaEdgeThreshold;
    float FxaaEdgeThresholdMin;
    float2 SourceTexelSize;
};

Texture2D SceneColorTexture : register(t0);
Texture2D BloomTexture : register(t1);
SamplerState LinearSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 TonemapACES(float3 value)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((value * (a * value + b)) / (value * (c * value + d) + e));
}

float3 LinearToSrgb(float3 value)
{
    return pow(saturate(value), 1.0f / 2.2f);
}

float3 SamplePostColor(float2 uv)
{
    const float3 sceneColor = SceneColorTexture.Sample(LinearSampler, uv).rgb;
    const float3 bloomColor = BloomTexture.Sample(LinearSampler, uv).rgb * BloomIntensity;
    const float3 hdrColor = max((sceneColor + bloomColor) * Exposure, 0.0f);
    return TonemapACES(hdrColor);
}

float Luma(float3 value)
{
    return dot(value, float3(0.299f, 0.587f, 0.114f));
}

float4 main(VSOutput input) : SV_TARGET
{
    float3 mappedColor = SamplePostColor(input.texcoord);

    if (FxaaEnabled > 0.5f)
    {
        const float2 texel = SourceTexelSize;
        const float3 rgbNW = SamplePostColor(input.texcoord + float2(-1.0f, -1.0f) * texel);
        const float3 rgbNE = SamplePostColor(input.texcoord + float2( 1.0f, -1.0f) * texel);
        const float3 rgbSW = SamplePostColor(input.texcoord + float2(-1.0f,  1.0f) * texel);
        const float3 rgbSE = SamplePostColor(input.texcoord + float2( 1.0f,  1.0f) * texel);
        const float3 rgbM = mappedColor;

        const float lumaNW = Luma(rgbNW);
        const float lumaNE = Luma(rgbNE);
        const float lumaSW = Luma(rgbSW);
        const float lumaSE = Luma(rgbSE);
        const float lumaM = Luma(rgbM);

        const float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
        const float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
        const float lumaRange = lumaMax - lumaMin;

        if (lumaRange >= max(FxaaEdgeThresholdMin, lumaMax * FxaaEdgeThreshold))
        {
            float2 dir;
            dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
            dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

            const float dirReduce = max(
                (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25f * FxaaSubpixel),
                1.0f / 128.0f);
            const float rcpDirMin = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);
            dir = clamp(dir * rcpDirMin, -8.0f, 8.0f) * texel;

            const float3 rgbA =
                0.5f * (
                    SamplePostColor(input.texcoord + dir * (1.0f / 3.0f - 0.5f)) +
                    SamplePostColor(input.texcoord + dir * (2.0f / 3.0f - 0.5f)));

            const float3 rgbB =
                rgbA * 0.5f +
                0.25f * (
                    SamplePostColor(input.texcoord + dir * -0.5f) +
                    SamplePostColor(input.texcoord + dir * 0.5f));

            const float lumaB = Luma(rgbB);
            mappedColor = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
        }
    }

    return float4(LinearToSrgb(mappedColor), 1.0f);
}
