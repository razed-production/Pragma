cbuffer SkyConstants : register(b0)
{
    float3 CameraForward;
    float TanHalfFovY;
    float3 CameraRight;
    float AspectRatio;
    float3 CameraUp;
    float SunIntensity;
    float3 SunDirection;
    float Padding0;
    float3 ZenithColor;
    float Padding1;
    float3 HorizonColor;
    float Padding2;
    float3 GroundColor;
    float Padding3;
    float3 SunColor;
    float SunAngularSize;
    float HorizonGlowStrength;
    float AtmosphereDensity;
    float GroundBounceStrength;
    float Padding4;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 SrgbToLinear(float3 value)
{
    return pow(saturate(value), 2.2f);
}

float4 main(VSOutput input) : SV_TARGET
{
    const float2 ndc = float2(input.texcoord.x * 2.0f - 1.0f, 1.0f - input.texcoord.y * 2.0f);
    const float3 rayDirection = normalize(
        CameraForward +
        CameraRight * (ndc.x * AspectRatio * TanHalfFovY) +
        CameraUp * (ndc.y * TanHalfFovY));

    const float upFactor = saturate(rayDirection.y * 0.5f + 0.5f);
    const float horizonFactor = saturate(1.0f - abs(rayDirection.y));
    const float atmosphereDensity = saturate(AtmosphereDensity);

    float3 skyColor = lerp(SrgbToLinear(GroundColor), SrgbToLinear(HorizonColor), saturate(rayDirection.y * 4.0f + 0.5f));
    skyColor = lerp(skyColor, SrgbToLinear(ZenithColor), pow(upFactor, 0.55f));
    skyColor += SrgbToLinear(HorizonColor) * pow(horizonFactor, lerp(10.0f, 4.0f, atmosphereDensity)) * HorizonGlowStrength;

    const float sunDot = saturate(dot(rayDirection, normalize(SunDirection)));
    const float sunDisc = smoothstep(1.0f - SunAngularSize * 12.0f, 1.0f - SunAngularSize, sunDot);
    const float sunGlow = pow(sunDot, lerp(96.0f, 42.0f, atmosphereDensity));
    const float sunScatter = pow(sunDot, lerp(28.0f, 9.0f, atmosphereDensity));
    skyColor += SrgbToLinear(SunColor) * SunIntensity * (sunDisc * 1.8f + sunGlow * 0.32f + sunScatter * 0.18f);
    skyColor += SrgbToLinear(SunColor) * SunIntensity * saturate(-rayDirection.y) * horizonFactor * GroundBounceStrength;

    return float4(max(skyColor, 0.0f), 1.0f);
}
