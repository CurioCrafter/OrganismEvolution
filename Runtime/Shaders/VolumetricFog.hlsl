// VolumetricFog.hlsl - Volumetric fog with light scattering
// Ray-marching based volumetric lighting

#include "Common.hlsl"

// Volumetric Constants
cbuffer VolumetricConstants : register(b0)
{
    float4x4 invViewProj;
    float3 cameraPos;
    float fogDensity;
    float3 lightDir;
    float scatteringCoeff;
    float3 lightColor;
    float absorptionCoeff;
    float2 screenSize;
    int numSteps;
    float maxDistance;
    float mieG;  // Mie scattering asymmetry parameter
    float3 padding;
};

// Input textures
Texture2D<float> depthTexture : register(t0);

// Output
RWTexture2D<float4> volumetricOutput : register(u0);

SamplerState pointSampler : register(s0);

// Mie phase function (forward scattering)
float MiePhase(float cosTheta, float g)
{
    float g2 = g * g;
    float phase = (1.0 - g2) / (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
    return phase;
}

// Reconstruct world position from depth
float3 ReconstructWorldPos(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;  // Flip Y for DirectX
    float4 worldPos = mul(invViewProj, clipPos);
    return worldPos.xyz / worldPos.w;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Volumetric fog runs at half resolution for performance
    uint2 pixelPos = dispatchThreadID.xy;

    if (pixelPos.x >= uint(screenSize.x / 2) || pixelPos.y >= uint(screenSize.y / 2))
        return;

    // Calculate full-res UV
    float2 uv = (float2(pixelPos) * 2.0 + 0.5) / screenSize;

    // Sample depth
    float depth = depthTexture.SampleLevel(pointSampler, uv, 0);

    // Reconstruct world position
    float3 worldPos = ReconstructWorldPos(uv, depth);

    // Ray direction and length
    float3 rayDir = normalize(worldPos - cameraPos);
    float rayLength = min(length(worldPos - cameraPos), maxDistance);

    // Ray-march through volume
    float stepSize = rayLength / float(numSteps);
    float3 rayStep = rayDir * stepSize;

    float3 currentPos = cameraPos;
    float3 scattering = float3(0, 0, 0);
    float transmittance = 1.0;

    float cosTheta = dot(rayDir, -lightDir);

    for (int i = 0; i < numSteps; ++i)
    {
        currentPos += rayStep;

        // Calculate density (can vary with height, noise, etc.)
        float density = fogDensity * exp(-currentPos.y * 0.01);  // Exponential height falloff

        // Calculate scattering
        float phase = MiePhase(cosTheta, mieG);
        float3 stepScattering = lightColor * density * scatteringCoeff * phase;

        // Integrate scattering
        scattering += stepScattering * transmittance * stepSize;

        // Update transmittance (Beer's law)
        transmittance *= exp(-density * absorptionCoeff * stepSize);

        // Early exit if fully opaque
        if (transmittance < 0.01)
            break;
    }

    // Output scattering contribution
    volumetricOutput[pixelPos] = float4(scattering, 1.0 - transmittance);
}
