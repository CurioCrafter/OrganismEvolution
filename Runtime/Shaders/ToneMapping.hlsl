// ToneMapping.hlsl - HDR to LDR tone mapping with color grading
// Final composition pass combining all post-process effects

#include "Common.hlsl"

// Tone Mapping Constants
cbuffer ToneMappingConstants : register(b0)
{
    float exposure;
    float gamma;
    float saturation;
    float contrast;
    float3 lift;
    float padding1;
    float3 gain;
    float bloomIntensity;
    float2 screenSize;
    uint enableSSAO;
    uint enableBloom;
    uint enableSSR;
    uint enableVolumetrics;
    uint enableFXAA;
    uint enableDistanceFog;
    float fxaaSubpixelQuality;
    float fxaaEdgeThreshold;
    float fxaaEdgeThresholdMin;
    float fogStart;
    float fogEnd;
    float fogDensity;
    float3 fogColor;
    float padding2;
};

// Input textures
Texture2D<float4> hdrTexture : register(t0);
Texture2D<float> ssaoTexture : register(t1);
Texture2D<float4> bloomTexture : register(t2);
Texture2D<float4> ssrTexture : register(t3);
Texture2D<float4> volumetricsTexture : register(t4);

SamplerState linearSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Fullscreen triangle vertex shader
VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate fullscreen triangle
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);

    return output;
}

// ACES tone mapping curve
float3 ACESFilm(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Uncharted 2 tone mapping
float3 Uncharted2Tonemap(float3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Vignette effect
float Vignette(float2 uv, float intensity, float radius)
{
    float2 center = uv - 0.5;
    float dist = length(center);
    return 1.0 - smoothstep(radius, radius + 0.3, dist) * intensity;
}

// Color grading
float3 ColorGrade(float3 color)
{
    // Apply lift/gamma/gain
    color = pow(max(color + lift, 0.0), 1.0 / gamma) * gain;

    // Saturation
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    color = lerp(luma.xxx, color, saturation);

    // Contrast
    color = (color - 0.5) * contrast + 0.5;

    return color;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.uv;

    // Sample HDR scene color
    float3 hdrColor = hdrTexture.Sample(linearSampler, uv).rgb;

    // Apply SSAO if enabled
    if (enableSSAO)
    {
        float ao = ssaoTexture.Sample(linearSampler, uv);
        hdrColor *= ao;
    }

    // Add bloom if enabled
    if (enableBloom)
    {
        float3 bloom = bloomTexture.Sample(linearSampler, uv).rgb;
        hdrColor += bloom * bloomIntensity;
    }

    // Add SSR if enabled
    if (enableSSR)
    {
        float4 ssr = ssrTexture.Sample(linearSampler, uv);
        hdrColor = lerp(hdrColor, ssr.rgb, ssr.a);
    }

    // Add volumetrics if enabled
    if (enableVolumetrics)
    {
        float4 volumetrics = volumetricsTexture.Sample(linearSampler, uv);
        hdrColor += volumetrics.rgb * volumetrics.a;
    }

    // Exposure adjustment
    hdrColor *= exposure;

    // Tone mapping (ACES)
    float3 ldrColor = ACESFilm(hdrColor);

    // Color grading
    ldrColor = ColorGrade(ldrColor);

    // Vignette (subtle)
    float vignette = Vignette(uv, 0.3, 0.7);
    ldrColor *= vignette;

    // Gamma correction
    ldrColor = pow(max(ldrColor, 0.0), 1.0 / 2.2);

    return float4(ldrColor, 1.0);
}
