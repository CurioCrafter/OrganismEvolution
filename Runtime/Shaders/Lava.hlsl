// =============================================================================
// Lava.hlsl - Volcanic lava rendering shader
// =============================================================================
// Used for rendering lava flows, lava particles, and volcanic effects
// Features procedural animation, heat distortion, and glowing emissive

// =============================================================================
// Constant Buffers
// =============================================================================

cbuffer LavaConstants : register(b0)
{
    float4x4 WorldViewProj;
    float4x4 World;
    float4 CameraPosition;
    float4 LavaParams;      // x: time, y: temperature (0-1), z: flow speed, w: glow intensity
    float4 ColorHot;        // RGB + emission strength
    float4 ColorCool;       // RGB + crust darkness
    float4 NoiseParams;     // x: scale, y: octaves, z: persistence, w: lacunarity
};

// =============================================================================
// Structures
// =============================================================================

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;      // Instance color/temperature
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float4 Color : TEXCOORD3;
    float Temperature : TEXCOORD4;
};

// =============================================================================
// Noise Functions for Procedural Lava
// =============================================================================

// Hash function for noise
float hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// 2D Perlin-style noise
float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float2 u = f * f * (3.0 - 2.0 * f); // Smoothstep

    float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// Fractal Brownian Motion for detailed lava texture
float fbm(float2 p, int octaves, float persistence, float lacunarity)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noise(p * frequency);
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value;
}

// Voronoi for cracked/cooled lava appearance
float voronoi(float2 p)
{
    float2 n = floor(p);
    float2 f = frac(p);

    float minDist = 8.0;

    for (int j = -1; j <= 1; j++)
    {
        for (int i = -1; i <= 1; i++)
        {
            float2 g = float2(i, j);
            float2 o = hash(n + g) * float2(1.0, 1.0);
            float2 r = g + o - f;
            float d = dot(r, r);
            minDist = min(minDist, d);
        }
    }

    return sqrt(minDist);
}

// =============================================================================
// Vertex Shader
// =============================================================================

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float time = LavaParams.x;
    float flowSpeed = LavaParams.z;

    // Animate vertex position for flowing lava
    float3 pos = input.Position;
    float2 flowOffset = float2(time * flowSpeed * 0.5, time * flowSpeed * 0.3);

    // Add wave motion to surface
    float waveHeight = fbm(input.TexCoord * 3.0 + flowOffset, 3, 0.5, 2.0) * 0.2;
    pos.y += waveHeight * input.Color.a; // Use color alpha as intensity

    // Transform
    output.Position = mul(float4(pos, 1.0), WorldViewProj);
    output.WorldPos = mul(float4(pos, 1.0), World).xyz;
    output.Normal = normalize(mul(float4(input.Normal, 0.0), World).xyz);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    output.Temperature = input.Color.r; // Use red channel as temperature

    return output;
}

// =============================================================================
// Pixel Shader
// =============================================================================

float4 PSMain(VSOutput input) : SV_Target
{
    float time = LavaParams.x;
    float baseTemp = LavaParams.y;
    float glowIntensity = LavaParams.w;

    // Animated UV for flowing effect
    float2 uv = input.TexCoord;
    float2 flowOffset = float2(time * 0.1, time * 0.08);

    // Temperature variation across surface
    float tempVariation = fbm(uv * NoiseParams.x + flowOffset,
                              (int)NoiseParams.y,
                              NoiseParams.z,
                              NoiseParams.w);

    // Combine with base temperature
    float temperature = saturate(baseTemp * input.Temperature + tempVariation * 0.3);

    // Cooled crust pattern using Voronoi
    float crustPattern = voronoi(uv * 5.0 + flowOffset * 0.5);
    float isCrust = smoothstep(0.1, 0.3, crustPattern);

    // Temperature affected by crust
    temperature = lerp(temperature, temperature * 0.3, isCrust * (1.0 - temperature));

    // Color blending based on temperature
    float3 hotColor = ColorHot.rgb;
    float3 coolColor = ColorCool.rgb;

    // Create color gradient: cool (dark red) -> warm (orange) -> hot (yellow-white)
    float3 color;
    if (temperature < 0.5)
    {
        // Cool to warm
        color = lerp(coolColor, float3(1.0, 0.4, 0.0), temperature * 2.0);
    }
    else
    {
        // Warm to hot
        color = lerp(float3(1.0, 0.4, 0.0), hotColor, (temperature - 0.5) * 2.0);
    }

    // Add bright spots for very hot areas
    float brightSpots = fbm(uv * 8.0 - flowOffset * 2.0, 2, 0.6, 2.0);
    brightSpots = smoothstep(0.6, 0.9, brightSpots) * temperature;
    color += float3(1.0, 0.9, 0.7) * brightSpots * 0.5;

    // Emissive glow - lava emits its own light
    float emission = temperature * glowIntensity * ColorHot.a;
    emission += brightSpots * 2.0;

    // Darken crust areas
    color *= lerp(1.0, ColorCool.a, isCrust * (1.0 - temperature * 0.5));

    // Rim glow for volumetric appearance
    float3 viewDir = normalize(CameraPosition.xyz - input.WorldPos);
    float rimFactor = 1.0 - saturate(dot(viewDir, input.Normal));
    rimFactor = pow(rimFactor, 2.0);
    color += hotColor * rimFactor * temperature * 0.3;

    // Heat shimmer distortion (passed to post-process)
    // float heatDistortion = temperature * fbm(uv * 20.0 + time * 2.0, 2, 0.5, 2.0);

    // Final color with HDR emission
    float3 finalColor = color + color * emission;

    // Alpha based on temperature and distance
    float alpha = lerp(0.8, 1.0, temperature);

    return float4(finalColor, alpha);
}

// =============================================================================
// Lava Particle Vertex Shader
// =============================================================================

struct ParticleVSInput
{
    float3 Position : POSITION;
    float2 Size : TEXCOORD0;      // x: size, y: rotation
    float4 Color : COLOR0;        // RGB + temperature
    float Life : TEXCOORD1;       // 0-1 lifetime
};

struct ParticleVSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float Temperature : TEXCOORD2;
    float Life : TEXCOORD3;
};

// Billboard particle expansion happens in geometry shader or instanced quads
// This is a simplified version for point sprites or pre-billboarded quads

ParticleVSOutput VSParticle(ParticleVSInput input, uint vertexID : SV_VertexID)
{
    ParticleVSOutput output;

    // Billboard quad corners
    static const float2 corners[4] = {
        float2(-1, -1),
        float2( 1, -1),
        float2(-1,  1),
        float2( 1,  1)
    };

    float2 corner = corners[vertexID % 4];

    // Calculate billboard vectors
    float3 worldPos = input.Position;
    float3 toCamera = normalize(CameraPosition.xyz - worldPos);
    float3 up = float3(0, 1, 0);
    float3 right = normalize(cross(toCamera, up));
    up = normalize(cross(right, toCamera));

    // Apply rotation
    float rot = input.Size.y;
    float cosR = cos(rot);
    float sinR = sin(rot);
    float2 rotatedCorner;
    rotatedCorner.x = corner.x * cosR - corner.y * sinR;
    rotatedCorner.y = corner.x * sinR + corner.y * cosR;

    // Scale and position
    float size = input.Size.x * (0.5 + input.Life * 0.5); // Shrink as it cools
    float3 vertexPos = worldPos + right * rotatedCorner.x * size + up * rotatedCorner.y * size;

    output.Position = mul(float4(vertexPos, 1.0), WorldViewProj);
    output.TexCoord = corner * 0.5 + 0.5;
    output.Color = input.Color;
    output.Temperature = input.Color.a;
    output.Life = input.Life;

    return output;
}

// =============================================================================
// Lava Particle Pixel Shader
// =============================================================================

float4 PSParticle(ParticleVSOutput input) : SV_Target
{
    // Distance from center for circular particle
    float2 centered = input.TexCoord * 2.0 - 1.0;
    float dist = length(centered);

    // Soft circle falloff
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);

    // Fade based on lifetime
    alpha *= input.Life;

    // Temperature-based color
    float temp = input.Temperature * input.Life;

    float3 color;
    if (temp > 0.7)
    {
        // Hot: yellow-white
        color = lerp(float3(1.0, 0.6, 0.1), float3(1.0, 1.0, 0.8), (temp - 0.7) / 0.3);
    }
    else if (temp > 0.4)
    {
        // Warm: orange
        color = lerp(float3(1.0, 0.2, 0.0), float3(1.0, 0.6, 0.1), (temp - 0.4) / 0.3);
    }
    else
    {
        // Cool: dark red
        color = lerp(float3(0.2, 0.0, 0.0), float3(1.0, 0.2, 0.0), temp / 0.4);
    }

    // Emission glow
    float emission = temp * 2.0;
    color = color * (1.0 + emission);

    // Core glow
    float core = 1.0 - smoothstep(0.0, 0.3, dist);
    color += float3(1.0, 0.9, 0.7) * core * temp;

    // Clip nearly invisible pixels
    clip(alpha - 0.01);

    return float4(color, alpha);
}

// =============================================================================
// Pyroclastic Cloud Shader
// =============================================================================

struct CloudVSOutput
{
    float4 Position : SV_Position;
    float3 WorldPos : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float Density : TEXCOORD2;
    float Temperature : TEXCOORD3;
};

float4 PSPyroclastic(CloudVSOutput input) : SV_Target
{
    float time = LavaParams.x;
    float2 uv = input.TexCoord;

    // Animated smoke/ash texture
    float2 offset1 = float2(time * 0.1, time * 0.05);
    float2 offset2 = float2(-time * 0.08, time * 0.12);

    float noise1 = fbm(uv * 4.0 + offset1, 4, 0.5, 2.0);
    float noise2 = fbm(uv * 6.0 + offset2, 3, 0.6, 2.2);

    float cloudDensity = (noise1 + noise2) * 0.5 * input.Density;

    // Hot core visible through cloud
    float temp = input.Temperature;
    float3 ashColor = float3(0.15, 0.12, 0.1);
    float3 hotColor = float3(1.0, 0.3, 0.0);

    float3 color = lerp(ashColor, hotColor, temp * noise1);

    // Edge fade for volumetric look
    float edgeFade = smoothstep(0.0, 0.3, cloudDensity) * smoothstep(1.0, 0.7, cloudDensity);

    // Inner glow
    color += hotColor * temp * 0.3 * (1.0 - noise2);

    float alpha = cloudDensity * edgeFade * 0.8;

    return float4(color, alpha);
}

// =============================================================================
// Technique Definitions (for reference)
// =============================================================================
/*
technique11 LavaSurface
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VSMain()));
        SetPixelShader(CompileShader(ps_5_0, PSMain()));
    }
}

technique11 LavaParticles
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VSParticle()));
        SetPixelShader(CompileShader(ps_5_0, PSParticle()));
    }
}

technique11 PyroclasticCloud
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VSMain()));
        SetPixelShader(CompileShader(ps_5_0, PSPyroclastic()));
    }
}
*/
