// Weather Effects Shader - Rain, Snow, and Atmospheric Effects
// GPU particle-based precipitation system with volumetric integration

#define THREAD_GROUP_SIZE 256
#define MAX_PARTICLES 65536

// ============================================================================
// Constant Buffers
// ============================================================================

cbuffer WeatherConstants : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float3 cameraPos;
    float time;

    // Weather parameters
    float3 windDirection;           // Normalized wind direction
    float windStrength;             // 0-1 wind intensity
    float precipitationIntensity;   // 0-1 precipitation amount
    float precipitationType;        // 0 = rain, 1 = snow
    float fogDensity;
    float fogHeight;

    // Precipitation bounds (spawn box around camera)
    float3 boundsMin;
    float spawnHeight;
    float3 boundsMax;
    float groundLevel;

    // Visual parameters
    float3 rainColor;
    float rainAlpha;
    float3 snowColor;
    float snowAlpha;

    // Lightning
    float lightningIntensity;
    float3 lightningPos;
};

// ============================================================================
// Particle Structure
// ============================================================================

struct Particle {
    float3 position;
    float3 velocity;
    float life;         // 0-1, 0 = dead
    float size;
    float rotation;     // For snow tumbling
    float type;         // 0 = rain, 1 = snow
};

// ============================================================================
// Resources
// ============================================================================

RWStructuredBuffer<Particle> particles : register(u0);
RWBuffer<uint> aliveCount : register(u1);

Texture2D<float> depthTexture : register(t0);
Texture2D<float4> sceneTexture : register(t1);
SamplerState linearSampler : register(s0);

// ============================================================================
// Noise Functions
// ============================================================================

float hash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    return lerp(lerp(lerp(hash(i + float3(0, 0, 0)), hash(i + float3(1, 0, 0)), f.x),
                     lerp(hash(i + float3(0, 1, 0)), hash(i + float3(1, 1, 0)), f.x), f.y),
                lerp(lerp(hash(i + float3(0, 0, 1)), hash(i + float3(1, 0, 1)), f.x),
                     lerp(hash(i + float3(0, 1, 1)), hash(i + float3(1, 1, 1)), f.x), f.y), f.z);
}

// ============================================================================
// Particle Simulation Compute Shader
// ============================================================================

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSSimulate(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint particleIndex = dispatchThreadID.x;

    if (particleIndex >= MAX_PARTICLES)
        return;

    Particle p = particles[particleIndex];

    float dt = 0.016f; // Fixed timestep (~60 FPS)

    // Skip dead particles
    if (p.life <= 0.0)
    {
        // Respawn if precipitation is active
        if (precipitationIntensity > 0.0)
        {
            // Use particle index for deterministic randomness
            float3 randomSeed = float3(particleIndex, time, particleIndex * 0.1234);
            float randX = noise3D(randomSeed);
            float randZ = noise3D(randomSeed + 100.0);
            float randY = noise3D(randomSeed + 200.0);

            // Random position in spawn box around camera
            float spawnRadius = 50.0; // Radius around camera
            p.position.x = cameraPos.x + (randX - 0.5) * spawnRadius * 2.0;
            p.position.z = cameraPos.z + (randZ - 0.5) * spawnRadius * 2.0;
            p.position.y = cameraPos.y + spawnHeight + randY * 10.0;

            // Set type based on weather
            p.type = precipitationType;

            // Initial velocity
            if (p.type < 0.5)
            {
                // Rain - falls fast
                p.velocity = float3(windDirection.x * windStrength * 5.0, -12.0, windDirection.z * windStrength * 5.0);
                p.size = 0.02 + randX * 0.02;
            }
            else
            {
                // Snow - falls slow, more affected by wind
                p.velocity = float3(windDirection.x * windStrength * 3.0, -1.0 - randY, windDirection.z * windStrength * 3.0);
                p.size = 0.03 + randX * 0.03;
            }

            p.rotation = randX * 6.28318;
            p.life = 1.0;

            // Only spawn based on precipitation intensity
            if (noise3D(randomSeed + 300.0) > precipitationIntensity * 0.5)
            {
                p.life = 0.0; // Don't spawn this frame
            }
        }
    }
    else
    {
        // Update particle physics

        // Gravity
        float gravity = (p.type < 0.5) ? -15.0 : -2.0;
        p.velocity.y += gravity * dt;

        // Wind variation
        float windNoise = noise3D(p.position * 0.1 + time * 0.5);
        float3 windEffect = float3(
            windDirection.x * windStrength * (1.0 + windNoise * 0.5),
            0.0,
            windDirection.z * windStrength * (1.0 + windNoise * 0.5)
        );

        // Snow tumbles more
        if (p.type >= 0.5)
        {
            windEffect *= 2.0;
            p.rotation += dt * 3.0;

            // Snow drifts sideways
            float drift = sin(time * 2.0 + p.position.x * 0.5) * 0.5;
            windEffect.x += drift;
        }

        p.velocity.xz += windEffect.xz * dt * 5.0;

        // Air resistance
        float drag = (p.type < 0.5) ? 0.02 : 0.1;
        p.velocity *= (1.0 - drag);

        // Update position
        p.position += p.velocity * dt;

        // Check if hit ground
        if (p.position.y < groundLevel)
        {
            p.life = 0.0;
        }

        // Check if too far from camera
        float3 toCamera = p.position - cameraPos;
        if (dot(toCamera.xz, toCamera.xz) > 3000.0)
        {
            p.life = 0.0;
        }

        // Decrease life slowly (for fading)
        p.life -= dt * 0.1;
    }

    particles[particleIndex] = p;
}

// ============================================================================
// Rendering Structures
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    uint instanceID : SV_InstanceID;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
    float type : TEXCOORD1;
};

// ============================================================================
// Vertex Shader - Billboard particles facing camera
// ============================================================================

StructuredBuffer<Particle> particleBuffer : register(t0);

PSInput VSMain(VSInput input)
{
    PSInput output;

    Particle p = particleBuffer[input.instanceID];

    if (p.life <= 0.0)
    {
        // Dead particle - render off-screen
        output.position = float4(-10000, -10000, 0, 1);
        output.texCoord = float2(0, 0);
        output.color = float4(0, 0, 0, 0);
        output.type = 0;
        return output;
    }

    // Calculate camera-facing billboard
    float3 cameraRight = float3(view[0][0], view[1][0], view[2][0]);
    float3 cameraUp = float3(view[0][1], view[1][1], view[2][1]);

    // Rain is elongated in fall direction, snow is round
    float3 quadPos;
    if (p.type < 0.5)
    {
        // Rain - stretched vertically
        float3 fallDir = normalize(p.velocity);
        float stretchFactor = length(p.velocity) * 0.1;

        // Elongate in velocity direction
        float3 right = normalize(cross(fallDir, cameraRight)) * p.size;
        float3 up = fallDir * p.size * (2.0 + stretchFactor);

        quadPos = p.position +
                  right * (input.texCoord.x - 0.5) * 2.0 +
                  up * (input.texCoord.y - 0.5) * 2.0;
    }
    else
    {
        // Snow - round, tumbling
        float c = cos(p.rotation);
        float s = sin(p.rotation);
        float2 rotatedUV = float2(
            c * (input.texCoord.x - 0.5) - s * (input.texCoord.y - 0.5),
            s * (input.texCoord.x - 0.5) + c * (input.texCoord.y - 0.5)
        );

        quadPos = p.position +
                  cameraRight * rotatedUV.x * p.size * 2.0 +
                  cameraUp * rotatedUV.y * p.size * 2.0;
    }

    output.position = mul(viewProjection, float4(quadPos, 1.0));
    output.texCoord = input.texCoord;
    output.type = p.type;

    // Color based on type
    if (p.type < 0.5)
    {
        output.color = float4(rainColor, rainAlpha * p.life);
    }
    else
    {
        output.color = float4(snowColor, snowAlpha * p.life);
    }

    // Lightning flash brightens precipitation
    if (lightningIntensity > 0.1)
    {
        output.color.rgb += lightningIntensity * 0.5;
    }

    return output;
}

// ============================================================================
// Pixel Shader - Render precipitation particles
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    // Rain drops are elongated with gradient
    // Snow flakes are round with soft edges

    float2 uv = input.texCoord * 2.0 - 1.0;
    float dist = length(uv);

    float alpha;
    if (input.type < 0.5)
    {
        // Rain - sharp horizontal edges, soft vertical
        float horizontal = 1.0 - abs(uv.x);
        float vertical = 1.0 - abs(uv.y);
        horizontal = pow(horizontal, 0.5);
        vertical = pow(vertical, 2.0);
        alpha = horizontal * vertical;
    }
    else
    {
        // Snow - soft circular
        alpha = saturate(1.0 - dist);
        alpha = alpha * alpha;

        // Add some sparkle
        float sparkle = noise3D(float3(input.texCoord * 10.0, time * 5.0));
        alpha *= 0.8 + sparkle * 0.4;
    }

    float4 color = input.color;
    color.a *= alpha;

    // Early out for invisible pixels
    if (color.a < 0.01)
    {
        discard;
    }

    return color;
}

// ============================================================================
// Wet Surface Effect (Applied to terrain after rain)
// ============================================================================

cbuffer WetSurfaceConstants : register(b1)
{
    float wetness;          // 0-1 ground wetness
    float puddleAmount;     // 0-1 puddle coverage
    float rippleIntensity;  // Ripple animation intensity
    float wetTime;          // Time for animation
};

float4 ApplyWetSurface(float4 baseColor, float3 worldPos, float3 normal, float3 viewDir)
{
    if (wetness <= 0.0)
        return baseColor;

    // Darken wet surfaces
    float3 wetColor = baseColor.rgb * (1.0 - wetness * 0.3);

    // Add specular reflection to wet surfaces
    float fresnel = pow(1.0 - saturate(dot(normal, viewDir)), 5.0);
    fresnel *= wetness;

    // Simplified sky reflection
    float3 reflection = float3(0.5, 0.6, 0.7) * fresnel * 0.3;
    wetColor += reflection;

    // Puddles in low areas (would need height data in practice)
    float puddleFactor = saturate(puddleAmount - noise3D(worldPos * 0.5) * 0.5);
    if (puddleFactor > 0.0)
    {
        // Puddle water color
        float3 puddleColor = float3(0.3, 0.35, 0.4);

        // Ripples from rain
        float2 rippleUV = worldPos.xz * 0.5;
        float ripple1 = sin(length(rippleUV - float2(wetTime * 0.3, 0.0)) * 5.0 - wetTime * 3.0);
        float ripple2 = sin(length(rippleUV - float2(0.0, wetTime * 0.4)) * 6.0 - wetTime * 3.5);
        float ripples = (ripple1 + ripple2) * 0.5 * rippleIntensity * puddleFactor;

        // Add ripple highlight
        puddleColor += ripples * 0.1;

        wetColor = lerp(wetColor, puddleColor, puddleFactor * 0.7);
    }

    return float4(wetColor, baseColor.a);
}

// ============================================================================
// Snow Accumulation Effect (Applied to terrain during snow)
// ============================================================================

float4 ApplySnowAccumulation(float4 baseColor, float3 worldPos, float3 normal, float snowAmount)
{
    if (snowAmount <= 0.0)
        return baseColor;

    // Snow accumulates on upward-facing surfaces
    float upFacing = saturate(normal.y);

    // More snow in sheltered areas (simplified)
    float shelterFactor = 1.0 - noise3D(worldPos * 0.1) * 0.3;

    // Calculate snow coverage
    float snowCoverage = saturate(snowAmount * upFacing * shelterFactor);

    // Snow color with slight variation
    float3 snowCol = float3(0.95, 0.95, 0.98);
    snowCol += (noise3D(worldPos * 5.0) - 0.5) * 0.05;

    // Blend with base
    float3 result = lerp(baseColor.rgb, snowCol, snowCoverage);

    // Sparkle on fresh snow
    float sparkle = noise3D(worldPos * 20.0 + wetTime * 0.5);
    if (sparkle > 0.95 && snowCoverage > 0.5)
    {
        result += 0.3;
    }

    return float4(result, baseColor.a);
}
