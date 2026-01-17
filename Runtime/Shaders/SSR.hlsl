// SSR.hlsl - Screen-Space Reflections
// Ray-marching based reflections using depth buffer

#include "Common.hlsl"

// SSR Constants
cbuffer SSRConstants : register(b0)
{
    float4x4 projection;
    float4x4 invProjection;
    float4x4 view;
    float2 screenSize;
    float maxDistance;
    float thickness;
    int maxSteps;
    int binarySearchSteps;
    float strideZCutoff;
    float padding;
};

// Input textures
Texture2D<float4> colorTexture : register(t0);
Texture2D<float> depthTexture : register(t1);
Texture2D<float4> normalTexture : register(t2);

// Output
RWTexture2D<float4> ssrOutput : register(u0);

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

// Reconstruct view position from depth
float3 ReconstructViewPos(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;  // Flip Y for DirectX
    float4 viewPos = mul(invProjection, clipPos);
    return viewPos.xyz / viewPos.w;
}

// Project view position to screen space
float3 ProjectToScreen(float3 viewPos)
{
    float4 clipPos = mul(projection, float4(viewPos, 1.0));
    clipPos.xyz /= clipPos.w;
    clipPos.y = -clipPos.y;
    clipPos.xy = clipPos.xy * 0.5 + 0.5;
    return clipPos.xyz;
}

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

    // Skip background or invalid normals
    if (depth >= 0.9999 || depth <= 0.0001 || length(normal) < 0.1)
    {
        ssrOutput[pixelPos] = float4(0, 0, 0, 0);
        return;
    }

    // Reconstruct view position
    float3 viewPos = ReconstructViewPos(uv, depth);

    // Calculate reflection direction
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, normal);

    // Ray-march in screen space
    float3 rayStart = viewPos;
    float3 rayEnd = rayStart + reflectDir * maxDistance;

    // Project to screen space
    float3 startScreen = ProjectToScreen(rayStart);
    float3 endScreen = ProjectToScreen(rayEnd);

    // Calculate step
    float2 delta = endScreen.xy - startScreen.xy;
    float steps = min(maxSteps, length(delta * screenSize));

    if (steps < 1.0)
    {
        ssrOutput[pixelPos] = float4(0, 0, 0, 0);
        return;
    }

    float2 step = delta / steps;

    // Ray-march
    float2 currentUV = startScreen.xy;
    bool hit = false;
    float hitAlpha = 0.0;

    for (int i = 0; i < int(steps); ++i)
    {
        currentUV += step;

        // Check bounds
        if (currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0)
            break;

        // Sample depth at current UV
        float sampleDepth = depthTexture.SampleLevel(pointSampler, currentUV, 0);

        // Reconstruct view Z
        float3 sampleViewPos = ReconstructViewPos(currentUV, sampleDepth);

        // Interpolate ray Z
        float t = float(i) / steps;
        float3 rayViewPos = lerp(rayStart, rayEnd, t);

        // Check for intersection
        float diff = rayViewPos.z - sampleViewPos.z;

        if (diff > 0.0 && diff < thickness)
        {
            hit = true;
            hitAlpha = 1.0 - t;  // Fade based on distance
            break;
        }
    }

    // Sample reflected color
    float4 reflectionColor = float4(0, 0, 0, 0);

    if (hit)
    {
        reflectionColor = colorTexture.SampleLevel(linearSampler, currentUV, 0);

        // Fade at screen edges
        float2 edgeFade = smoothstep(0.0, 0.1, currentUV) * (1.0 - smoothstep(0.9, 1.0, currentUV));
        float edgeFactor = edgeFade.x * edgeFade.y;

        reflectionColor.a = hitAlpha * edgeFactor;
    }

    ssrOutput[pixelPos] = reflectionColor;
}
