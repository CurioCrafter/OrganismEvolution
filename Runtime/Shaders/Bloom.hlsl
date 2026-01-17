// Bloom.hlsl - HDR Bloom post-process effect
// Extract bright pixels and progressively downsample/blur

#include "Common.hlsl"

// Bloom Constants
cbuffer BloomConstants : register(b0)
{
    float threshold;
    float intensity;
    float filterRadius;
    float padding;
    float2 texelSize;
    float2 padding2;
};

// Input/Output textures
Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

SamplerState linearSampler : register(s0);

// Luminance calculation
float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// Extract bright pixels above threshold
[numthreads(8, 8, 1)]
void CSExtract(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;
    uint2 outputSize;
    outputTexture.GetDimensions(outputSize.x, outputSize.y);

    if (pixelPos.x >= outputSize.x || pixelPos.y >= outputSize.y)
        return;

    float2 uv = (float2(pixelPos) + 0.5) * texelSize;

    // Sample HDR color
    float4 color = inputTexture.SampleLevel(linearSampler, uv, 0);

    // Extract bright pixels based on luminance threshold
    float luma = Luminance(color.rgb);
    float contribution = max(0.0, luma - threshold);
    float3 bloomColor = color.rgb * (contribution / (luma + 0.0001));

    outputTexture[pixelPos] = float4(bloomColor, 1.0);
}

// Downsample and blur (13-tap filter)
[numthreads(8, 8, 1)]
void CSDownsample(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;
    uint2 outputSize;
    outputTexture.GetDimensions(outputSize.x, outputSize.y);

    if (pixelPos.x >= outputSize.x || pixelPos.y >= outputSize.y)
        return;

    float2 uv = (float2(pixelPos) + 0.5) / float2(outputSize);

    // 13-tap downsampling filter
    float3 result = float3(0, 0, 0);

    // Center sample
    result += inputTexture.SampleLevel(linearSampler, uv, 0).rgb * 0.125;

    // 4 corner samples
    result += inputTexture.SampleLevel(linearSampler, uv + float2(-1, -1) * texelSize, 0).rgb * 0.125;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 1, -1) * texelSize, 0).rgb * 0.125;
    result += inputTexture.SampleLevel(linearSampler, uv + float2(-1,  1) * texelSize, 0).rgb * 0.125;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 1,  1) * texelSize, 0).rgb * 0.125;

    // 4 edge samples
    result += inputTexture.SampleLevel(linearSampler, uv + float2(-1,  0) * texelSize, 0).rgb * 0.0625;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 1,  0) * texelSize, 0).rgb * 0.0625;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0, -1) * texelSize, 0).rgb * 0.0625;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0,  1) * texelSize, 0).rgb * 0.0625;

    // 4 inner diagonal samples
    result += inputTexture.SampleLevel(linearSampler, uv + float2(-0.5, -0.5) * texelSize, 0).rgb * 0.0625;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0.5, -0.5) * texelSize, 0).rgb * 0.0625;
    result += inputTexture.SampleLevel(linearSampler, uv + float2(-0.5,  0.5) * texelSize, 0).rgb * 0.0625;
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0.5,  0.5) * texelSize, 0).rgb * 0.0625;

    outputTexture[pixelPos] = float4(result, 1.0);
}

// Upsample and combine (tent filter)
[numthreads(8, 8, 1)]
void CSUpsample(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;
    uint2 outputSize;
    outputTexture.GetDimensions(outputSize.x, outputSize.y);

    if (pixelPos.x >= outputSize.x || pixelPos.y >= outputSize.y)
        return;

    float2 uv = (float2(pixelPos) + 0.5) / float2(outputSize);

    // 9-tap tent filter for upsampling
    float3 result = float3(0, 0, 0);

    result += inputTexture.SampleLevel(linearSampler, uv + float2(-1, -1) * texelSize, 0).rgb * (1.0 / 16.0);
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0, -1) * texelSize, 0).rgb * (2.0 / 16.0);
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 1, -1) * texelSize, 0).rgb * (1.0 / 16.0);

    result += inputTexture.SampleLevel(linearSampler, uv + float2(-1,  0) * texelSize, 0).rgb * (2.0 / 16.0);
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0,  0) * texelSize, 0).rgb * (4.0 / 16.0);
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 1,  0) * texelSize, 0).rgb * (2.0 / 16.0);

    result += inputTexture.SampleLevel(linearSampler, uv + float2(-1,  1) * texelSize, 0).rgb * (1.0 / 16.0);
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 0,  1) * texelSize, 0).rgb * (2.0 / 16.0);
    result += inputTexture.SampleLevel(linearSampler, uv + float2( 1,  1) * texelSize, 0).rgb * (1.0 / 16.0);

    // Blend with existing content
    float3 existingColor = outputTexture[pixelPos].rgb;
    outputTexture[pixelPos] = float4(existingColor + result * intensity, 1.0);
}
