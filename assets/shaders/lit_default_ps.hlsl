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
    float ShadowFilterQuality;
    float ShadingQuality;
    float2 Padding1;
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

Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D OrmTexture : register(t2);
Texture2D EmissiveTexture : register(t3);
Texture2D ShadowTexture : register(t4);
SamplerState MaterialSampler : register(s0);
SamplerState ShadowSampler : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPosition : TEXCOORD1;
    float4 shadowPosition : TEXCOORD2;
};

float3 SrgbToLinear(float3 value)
{
    return pow(saturate(value), 2.2f);
}

float DistributionGGX(const float nDotH, const float roughness)
{
    const float alpha = roughness * roughness;
    const float alphaSquared = alpha * alpha;
    const float denominator = nDotH * nDotH * (alphaSquared - 1.0f) + 1.0f;
    return alphaSquared / max(3.1415926535f * denominator * denominator, 0.0001f);
}

float GeometrySchlickGGX(const float nDotV, const float roughness)
{
    const float r = roughness + 1.0f;
    const float k = (r * r) * 0.125f;
    return nDotV / lerp(nDotV, 1.0f, k);
}

float GeometrySmith(const float nDotV, const float nDotL, const float roughness)
{
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

float3 FresnelSchlick(const float cosTheta, const float3 f0)
{
    return f0 + (1.0f - f0) * pow(1.0f - cosTheta, 5.0f);
}

float3 EvaluateProceduralSky(float3 direction)
{
    const float3 zenithColor = SrgbToLinear(SkyZenithColor);
    const float3 horizonColor = SrgbToLinear(SkyHorizonColor);
    const float3 groundColor = SrgbToLinear(SkyGroundColor);
    const float3 sunColor = SrgbToLinear(LightColor);
    const float3 sunDirection = normalize(-LightDirection);

    const float upFactor = saturate(direction.y * 0.5f + 0.5f);
    const float horizonFactor = saturate(1.0f - abs(direction.y));
    const float atmosphereDensity = saturate(SkyAtmosphereDensity);

    float3 skyColor = lerp(groundColor, horizonColor, saturate(direction.y * 4.0f + 0.5f));
    skyColor = lerp(skyColor, zenithColor, pow(upFactor, 0.55f));
    skyColor += horizonColor * pow(horizonFactor, lerp(10.0f, 4.0f, atmosphereDensity)) * lerp(0.05f, 0.16f, atmosphereDensity);

    const float sunDot = saturate(dot(direction, sunDirection));
    const float sunScatter = pow(sunDot, lerp(32.0f, 10.0f, atmosphereDensity));
    skyColor += sunColor * LightIntensity * sunScatter * lerp(0.05f, 0.22f, atmosphereDensity);
    skyColor += sunColor * LightIntensity * saturate(-direction.y) * horizonFactor * 0.03f;
    return max(skyColor, 0.0f);
}

float3x3 ComputeCotangentFrame(const float3 normal, const float3 worldPosition, const float2 texcoord)
{
    const float3 dpdx = ddx(worldPosition);
    const float3 dpdy = ddy(worldPosition);
    const float2 duvdx = ddx(texcoord);
    const float2 duvdy = ddy(texcoord);

    const float3 dpdyPerp = cross(dpdy, normal);
    const float3 dpdxPerp = cross(normal, dpdx);
    float3 tangent = dpdyPerp * duvdx.x + dpdxPerp * duvdy.x;
    float3 bitangent = dpdyPerp * duvdx.y + dpdxPerp * duvdy.y;

    const float invMax = rsqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)) + 1e-8f);
    return float3x3(tangent * invMax, bitangent * invMax, normal);
}

float3 ApplyNormalTexture(const float3 normal, const float3 worldPosition, const float2 texcoord)
{
    const float3 sampled = NormalTexture.Sample(MaterialSampler, texcoord).xyz * 2.0f - 1.0f;
    const float2 scaledXY = sampled.xy * max(NormalStrength, 0.0f);
    const float3 tangentSpaceNormal = normalize(float3(scaledXY, sampled.z));
    const float3x3 tangentToWorld = ComputeCotangentFrame(normal, worldPosition, texcoord);
    return normalize(mul(tangentSpaceNormal, tangentToWorld));
}

float SampleShadowTerm(float4 shadowPosition)
{
    if (ShadowStrength <= 0.0f || shadowPosition.w <= 0.0f)
    {
        return 1.0f;
    }

    const float3 projected = shadowPosition.xyz / shadowPosition.w;
    const float2 uv = float2(projected.x * 0.5f + 0.5f, -projected.y * 0.5f + 0.5f);
    const float depth = projected.z;

    if (uv.x <= 0.0f || uv.x >= 1.0f || uv.y <= 0.0f || uv.y >= 1.0f || depth <= 0.0f || depth >= 1.0f)
    {
        return 1.0f;
    }

    if (ShadowFilterQuality < 0.5f)
    {
        const float storedDepth = ShadowTexture.Sample(ShadowSampler, uv).r;
        const float shadowVisibility = depth - ShadowBias <= storedDepth ? 1.0f : 0.0f;
        return lerp(1.0f, shadowVisibility, saturate(ShadowStrength));
    }

    float visibility = 0.0f;
    const float2 offset = ShadowMapTexelSize * 0.75f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < 4; ++sampleIndex)
    {
        float2 sampleOffset = float2(0.0f, 0.0f);
        if (sampleIndex == 0)
        {
            sampleOffset = float2(-offset.x, -offset.y);
        }
        else if (sampleIndex == 1)
        {
            sampleOffset = float2(offset.x, -offset.y);
        }
        else if (sampleIndex == 2)
        {
            sampleOffset = float2(-offset.x, offset.y);
        }
        else
        {
            sampleOffset = float2(offset.x, offset.y);
        }

        const float2 sampleUv = clamp(uv + sampleOffset, float2(0.001f, 0.001f), float2(0.999f, 0.999f));
        const float storedDepth = ShadowTexture.Sample(ShadowSampler, sampleUv).r;
        visibility += depth - ShadowBias <= storedDepth ? 1.0f : 0.0f;
    }

    const float shadowVisibility = visibility * 0.25f;
    return lerp(1.0f, shadowVisibility, saturate(ShadowStrength));
}

float ComputeHeightFog(float3 worldPosition)
{
    const float distanceToCamera = distance(CameraPosition, worldPosition);
    const float fogDistance = max(distanceToCamera - FogStartDistance, 0.0f);
    const float averageHeight = max((CameraPosition.y + worldPosition.y) * 0.5f, -8.0f);
    const float heightAttenuation = exp(-max(averageHeight, 0.0f) * max(FogHeightFalloff, 0.0001f));
    const float fogAmount = 1.0f - exp(-fogDistance * max(FogDensity, 0.0f) * max(heightAttenuation, 0.08f));
    return saturate(fogAmount) * saturate(FogMaxOpacity);
}

float4 main(PSInput input) : SV_TARGET
{
    const bool mediumShading = ShadingQuality >= 0.5f;
    const bool fullShading = ShadingQuality >= 1.5f;
    float3 normal = normalize(input.normal);
    if (mediumShading && UseNormalTexture > 0.5f)
    {
        normal = ApplyNormalTexture(normal, input.worldPosition, input.texcoord);
    }

    float3 lightDirection = normalize(-LightDirection);
    float3 viewDirection = normalize(CameraPosition - input.worldPosition);
    float3 halfVector = normalize(lightDirection + viewDirection);
    float3 albedo = SrgbToLinear(saturate(input.color * BaseColor.rgb));
    if (UseAlbedoTexture > 0.5f)
    {
        albedo *= SrgbToLinear(AlbedoTexture.Sample(MaterialSampler, input.texcoord).rgb);
    }

    float roughness = max(0.045f, saturate(Roughness));
    float metallic = saturate(Metallic);
    float ambientOcclusion = saturate(AmbientOcclusion);
    if (mediumShading && UseOrmTexture > 0.5f)
    {
        const float3 orm = OrmTexture.Sample(MaterialSampler, input.texcoord).rgb;
        ambientOcclusion *= saturate(orm.r);
        roughness = max(0.045f, saturate(orm.g));
        metallic = saturate(orm.b);
    }

    const float ndotl = saturate(dot(normal, lightDirection));
    const float ndotv = saturate(dot(normal, viewDirection));
    const float ndoth = saturate(dot(normal, halfVector));
    const float vdoth = saturate(dot(viewDirection, halfVector));
    const float3 skyAmbient = float3(0.36f, 0.46f, 0.62f);
    const float3 groundAmbient = float3(0.06f, 0.07f, 0.08f);
    const float shadowTerm = SampleShadowTerm(input.shadowPosition);
    const float3 lightRadiance = LightIntensity * shadowTerm * SrgbToLinear(LightColor);
    const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 fresnel = f0;
    float3 viewFresnel = f0;
    float3 specular = 0.0f;
    if (mediumShading)
    {
        fresnel = FresnelSchlick(vdoth, f0);
        viewFresnel = FresnelSchlick(ndotv, f0);
        const float distribution = DistributionGGX(ndoth, roughness);
        const float geometry = GeometrySmith(ndotv, ndotl, roughness);
        specular = (distribution * geometry * fresnel) / max(4.0f * ndotv * ndotl, 0.001f);
    }
    else
    {
        const float cheapSpecular = pow(ndoth, lerp(48.0f, 8.0f, roughness));
        fresnel = FresnelSchlick(vdoth, f0);
        viewFresnel = FresnelSchlick(ndotv, f0);
        specular = fresnel * cheapSpecular * 0.35f;
    }
    const float3 kS = fresnel;
    const float3 kD = (1.0f - kS) * (1.0f - metallic);
    const float3 ambientColor = lerp(groundAmbient, skyAmbient, saturate(normal.y * 0.5f + 0.5f)) * AmbientStrength;
    const float3 diffuseEnvironment = mediumShading ? EvaluateProceduralSky(normalize(normal)) : ambientColor * 1.45f;
    float3 specularEnvironment = ambientColor;
    if (fullShading)
    {
        const float3 reflectionDirection = normalize(reflect(-viewDirection, normal));
        const float3 mirrorEnvironment = EvaluateProceduralSky(reflectionDirection);
        specularEnvironment = lerp(mirrorEnvironment, ambientColor, saturate(roughness * roughness * 0.85f));
    }
    else if (mediumShading)
    {
        specularEnvironment = lerp(diffuseEnvironment, ambientColor, saturate(roughness * roughness * 0.85f));
    }
    const float3 diffuse = (kD * albedo) / 3.1415926535f;
    const float3 directLighting = (diffuse + specular) * lightRadiance * ndotl;
    const float3 ambientLighting =
        ambientColor * albedo * ambientOcclusion * (1.0f - metallic * 0.5f) +
        diffuseEnvironment * albedo * ambientOcclusion * kD * (EnvironmentDiffuseStrength * AmbientStrength * 0.85f);
    const float grazingBoost = mediumShading ? lerp(0.04f, 0.18f, 1.0f - roughness) : 0.06f;
    const float3 specularIbl = specularEnvironment * viewFresnel * ambientOcclusion * grazingBoost * EnvironmentSpecularStrength;
    float3 emissiveSample = float3(1.0f, 1.0f, 1.0f);
    if (mediumShading && UseEmissiveTexture > 0.5f)
    {
        emissiveSample = SrgbToLinear(EmissiveTexture.Sample(MaterialSampler, input.texcoord).rgb);
    }
    const float3 emissiveLighting = emissiveSample * SrgbToLinear(EmissiveColor) * max(EmissiveIntensity, 0.0f);

    float3 litColor = ambientLighting + specularIbl + directLighting + emissiveLighting;
    const float fogFactor = ComputeHeightFog(input.worldPosition);
    const float3 fogColorLinear = SrgbToLinear(FogColor);
    litColor = lerp(litColor, fogColorLinear, fogFactor);
    return float4(max(litColor, 0.0f), BaseColor.a);
}
