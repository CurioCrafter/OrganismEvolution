// SSAO.hlsl - Screen-Space Ambient Occlusion
// Implements SSAO using hemisphere sampling and bilateral blur

#include "Common.hlsl"

// SSAO Constants
cbuffer SSAOConstants : register(b0)
{
    float4x4 projection;
    float4x4 invProjection;
    float radius;
    float bias;
    float2 noiseScale;
    float2 screenSize;
    float intensity;
    float padding;
    float4 samples[32];  // Hemisphere kernel samples
};

// Input textures
Texture2D<float> depthTexture : register(t0);
Texture2D<float4> normalTexture : register(t1);
Texture2D<float4> noiseTexture : register(t2);

// Output
RWTexture2D<float> ssaoOutput : register(u0);

// Samplers
SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;

    if (pixelPos.x >= uint(screenSize.x) || pixelPos.y >= uint(screenSize.y))
        return;

    float2 uv = (float2(pixelPos) + 0.5) / screenSize;

    // Sample depth and normal
    float depth = depthTexture[pixelPos];
    float3 normal = normalTexture[pixelPos].xyz;

    // Skip background (depth = 1.0 or 0.0)
    if (depth >= 0.9999 || depth <= 0.0001)
    {
        ssaoOutput[pixelPos] = 1.0;
        return;
    }

    // Reconstruct view-space position from depth
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;  // Flip Y for DirectX
    float4 viewPos = mul(invProjection, clipPos);
    viewPos /= viewPos.w;

    // Sample noise for random rotation
    float2 noiseUV = uv * noiseScale;
    float3 randomVec = noiseTexture.SampleLevel(pointSampler, noiseUV, 0).xyz;

    // Create TBN matrix for hemisphere orientation
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    // Accumulate occlusion
    float occlusion = 0.0;
    for (int i = 0; i < 32; ++i)
    {
        // Get sample position in view space
        float3 sampleDir = mul(TBN, samples[i].xyz);
        float3 samplePos = viewPos.xyz + sampleDir * radius;

        // Project to screen space
        float4 offset = mul(projection, float4(samplePos, 1.0));
        offset.xyz /= offset.w;
        offset.y = -offset.y;
        offset.xy = offset.xy * 0.5 + 0.5;

        // Sample depth at offset position
        float sampleDepth = depthTexture.SampleLevel(pointSampler, offset.xy, 0);

        // Reconstruct sample view position
        float4 sampleClipPos = float4(offset.xy * 2.0 - 1.0, sampleDepth, 1.0);
        sampleClipPos.y = -sampleClipPos.y;
        float4 sampleViewPos = mul(invProjection, sampleClipPos);
        sampleViewPos /= sampleViewPos.w;

        // Range check
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewPos.z - sampleViewPos.z));

        // Increment occlusion if sample is occluded
        occlusion += (sampleViewPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / 32.0) * intensity;
    ssaoOutput[pixelPos] = occlusion;
}

// Bilateral blur to reduce noise
[numthreads(8, 8, 1)]
void CSBlur(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadID.xy;

    if (pixelPos.x >= uint(screenSize.x) || pixelPos.y >= uint(screenSize.y))
        return;

    float result = 0.0;
    float totalWeight = 0.0;

    // 3x3 bilateral blur
    const int blurRadius = 2;
    for (int x = -blurRadius; x <= blurRadius; ++x)
    {
        for (int y = -blurRadius; y <= blurRadius; ++y)
        {
            int2 samplePos = int2(pixelPos) + int2(x, y);
            samplePos = clamp(samplePos, int2(0, 0), int2(screenSize) - 1);

            float sampleAO = ssaoOutput[samplePos];
            float weight = 1.0 / (1.0 + abs(float(x)) + abs(float(y)));

            result += sampleAO * weight;
            totalWeight += weight;
        }
    }

    ssaoOutput[pixelPos] = result / totalWeight;
}
