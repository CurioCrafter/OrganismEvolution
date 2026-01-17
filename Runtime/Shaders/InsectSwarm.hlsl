// InsectSwarm.hlsl
// Shader for rendering large numbers of small creatures efficiently
// Supports instanced rendering, point sprites, and particle systems

// =============================================================================
// Common Structures and Constants
// =============================================================================

cbuffer FrameConstants : register(b0)
{
    float4x4 ViewProjection;
    float4x4 View;
    float4x4 Projection;
    float3 CameraPosition;
    float Time;
    float3 SunDirection;
    float DeltaTime;
    float2 ScreenSize;
    float2 _Padding;
};

cbuffer RenderSettings : register(b1)
{
    float4 AmbientColor;
    float4 SunColor;
    float DetailDistance;       // Distance for detailed LOD
    float SimplifiedDistance;   // Distance for simplified LOD
    float PointDistance;        // Distance for point sprites
    float FadeDistance;         // Distance to start fading
};

// =============================================================================
// Instanced Mesh Rendering
// =============================================================================

struct VS_INPUT_INSTANCED
{
    // Per-vertex data
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;

    // Per-instance data
    float4x4 World : INST_WORLD;
    float4 Color : INST_COLOR;
    float4 Params : INST_PARAMS;  // x=scale, y=animation, z=type, w=lod
};

struct VS_OUTPUT_INSTANCED
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float4 Color : COLOR0;
    float Animation : TEXCOORD3;
};

VS_OUTPUT_INSTANCED VS_Instanced(VS_INPUT_INSTANCED input)
{
    VS_OUTPUT_INSTANCED output;

    // Apply animation to position (simple leg movement)
    float3 localPos = input.Position;

    // Animate legs based on Y position (legs are lower than body)
    if (localPos.y < -0.1f)
    {
        float legAnim = sin(input.Params.y * 10.0f + localPos.x * 5.0f) * 0.05f;
        localPos.z += legAnim;
    }

    // Transform to world space
    float4 worldPos = mul(float4(localPos, 1.0f), input.World);
    output.WorldPos = worldPos.xyz;

    // Transform to clip space
    output.Position = mul(worldPos, ViewProjection);

    // Transform normal
    float3x3 normalMatrix = (float3x3)input.World;
    output.Normal = normalize(mul(input.Normal, normalMatrix));

    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    output.Animation = input.Params.y;

    return output;
}

float4 PS_Instanced(VS_OUTPUT_INSTANCED input) : SV_TARGET
{
    // Simple Lambert shading
    float3 N = normalize(input.Normal);
    float NdotL = max(0.0f, dot(N, -SunDirection));

    // Ambient + diffuse
    float3 diffuse = input.Color.rgb * (AmbientColor.rgb + SunColor.rgb * NdotL);

    // Rim lighting for small creatures
    float3 viewDir = normalize(CameraPosition - input.WorldPos);
    float rim = 1.0f - max(0.0f, dot(N, viewDir));
    rim = pow(rim, 3.0f) * 0.3f;
    diffuse += rim * input.Color.rgb;

    // Distance fade
    float dist = length(input.WorldPos - CameraPosition);
    float fade = saturate(1.0f - (dist - FadeDistance) / (PointDistance - FadeDistance));

    return float4(diffuse, input.Color.a * fade);
}

// =============================================================================
// Point Sprite Rendering (for distant creatures)
// =============================================================================

struct VS_INPUT_POINT
{
    float3 Position : POSITION;
    float Size : SIZE;
    float4 Color : COLOR;
};

struct GS_OUTPUT_POINT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
};

VS_INPUT_POINT VS_Point(VS_INPUT_POINT input)
{
    // Pass through to geometry shader
    return input;
}

[maxvertexcount(4)]
void GS_Point(point VS_INPUT_POINT input[1], inout TriangleStream<GS_OUTPUT_POINT> stream)
{
    // Calculate view-space position
    float4 viewPos = mul(float4(input[0].Position, 1.0f), View);

    // Scale size based on distance
    float dist = length(viewPos.xyz);
    float size = input[0].Size * (1.0f + dist * 0.01f);

    // Billboard corners
    float2 offsets[4] = {
        float2(-1, -1),
        float2(-1,  1),
        float2( 1, -1),
        float2( 1,  1)
    };

    float2 texCoords[4] = {
        float2(0, 1),
        float2(0, 0),
        float2(1, 1),
        float2(1, 0)
    };

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        GS_OUTPUT_POINT output;

        // Offset in view space
        float4 cornerPos = viewPos;
        cornerPos.xy += offsets[i] * size;

        // Project
        output.Position = mul(cornerPos, Projection);
        output.TexCoord = texCoords[i];
        output.Color = input[0].Color;

        stream.Append(output);
    }
}

float4 PS_Point(GS_OUTPUT_POINT input) : SV_TARGET
{
    // Circular point sprite
    float2 center = input.TexCoord - 0.5f;
    float dist = length(center);

    // Soft circle
    float alpha = 1.0f - smoothstep(0.3f, 0.5f, dist);

    // Color with alpha
    return float4(input.Color.rgb, input.Color.a * alpha);
}

// =============================================================================
// Swarm Particle System
// =============================================================================

struct VS_INPUT_PARTICLE
{
    float3 Position : POSITION;
    float3 Velocity : VELOCITY;
    float Size : SIZE;
    float Life : LIFE;
    float4 Color : COLOR;
};

struct GS_OUTPUT_PARTICLE
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    float Life : TEXCOORD1;
};

VS_INPUT_PARTICLE VS_Particle(VS_INPUT_PARTICLE input)
{
    return input;
}

[maxvertexcount(4)]
void GS_Particle(point VS_INPUT_PARTICLE input[1], inout TriangleStream<GS_OUTPUT_PARTICLE> stream)
{
    // Skip dead particles
    if (input[0].Life <= 0.0f)
        return;

    // Calculate view-space position
    float4 viewPos = mul(float4(input[0].Position, 1.0f), View);

    // Size based on life (shrink as dying)
    float lifeFactor = saturate(input[0].Life);
    float size = input[0].Size * lifeFactor;

    // Velocity-based stretch
    float3 velView = mul(float4(input[0].Velocity, 0.0f), View).xyz;
    float speed = length(velView);
    float stretch = 1.0f + speed * 0.1f;

    // Billboard with velocity stretch
    float2 offsets[4] = {
        float2(-1, -1),
        float2(-1,  1),
        float2( 1, -1),
        float2( 1,  1)
    };

    float2 texCoords[4] = {
        float2(0, 1),
        float2(0, 0),
        float2(1, 1),
        float2(1, 0)
    };

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        GS_OUTPUT_PARTICLE output;

        float4 cornerPos = viewPos;

        // Apply stretch in velocity direction
        float2 offset = offsets[i] * size;
        if (speed > 0.01f)
        {
            float2 velDir = normalize(velView.xy);
            offset.x *= stretch;

            // Rotate to velocity direction
            float2 rotOffset;
            rotOffset.x = offset.x * velDir.x - offset.y * velDir.y;
            rotOffset.y = offset.x * velDir.y + offset.y * velDir.x;
            offset = rotOffset;
        }

        cornerPos.xy += offset;

        output.Position = mul(cornerPos, Projection);
        output.TexCoord = texCoords[i];
        output.Color = input[0].Color;
        output.Life = input[0].Life;

        stream.Append(output);
    }
}

float4 PS_Particle(GS_OUTPUT_PARTICLE input) : SV_TARGET
{
    // Soft particle
    float2 center = input.TexCoord - 0.5f;
    float dist = length(center);

    // Fade edges
    float alpha = 1.0f - smoothstep(0.2f, 0.5f, dist);

    // Fade with life
    alpha *= saturate(input.Life);

    // Add slight glow
    float glow = exp(-dist * dist * 10.0f) * 0.5f;
    float3 color = input.Color.rgb + glow;

    return float4(color, input.Color.a * alpha);
}

// =============================================================================
// Pheromone Trail Rendering
// =============================================================================

struct VS_INPUT_TRAIL
{
    float3 Start : POSITION0;
    float3 End : POSITION1;
    float Strength : STRENGTH;
    float4 Color : COLOR;
};

struct GS_OUTPUT_TRAIL
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    float Strength : TEXCOORD1;
};

VS_INPUT_TRAIL VS_Trail(VS_INPUT_TRAIL input)
{
    return input;
}

[maxvertexcount(4)]
void GS_Trail(point VS_INPUT_TRAIL input[1], inout TriangleStream<GS_OUTPUT_TRAIL> stream)
{
    // Skip weak trails
    if (input[0].Strength < 0.05f)
        return;

    float3 start = input[0].Start;
    float3 end = input[0].End;

    // Line direction
    float3 lineDir = normalize(end - start);

    // Calculate perpendicular in screen space
    float4 startClip = mul(float4(start, 1.0f), ViewProjection);
    float4 endClip = mul(float4(end, 1.0f), ViewProjection);

    float2 startScreen = startClip.xy / startClip.w;
    float2 endScreen = endClip.xy / endClip.w;

    float2 screenDir = normalize(endScreen - startScreen);
    float2 perpScreen = float2(-screenDir.y, screenDir.x);

    // Width based on strength
    float width = 0.01f * input[0].Strength;

    // Offset in clip space
    float2 perpOffset = perpScreen * width;

    // Corners
    float4 corners[4];
    corners[0] = startClip;
    corners[0].xy += perpOffset * startClip.w;
    corners[1] = startClip;
    corners[1].xy -= perpOffset * startClip.w;
    corners[2] = endClip;
    corners[2].xy += perpOffset * endClip.w;
    corners[3] = endClip;
    corners[3].xy -= perpOffset * endClip.w;

    float2 texCoords[4] = {
        float2(0, 0),
        float2(0, 1),
        float2(1, 0),
        float2(1, 1)
    };

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        GS_OUTPUT_TRAIL output;
        output.Position = corners[i];
        output.TexCoord = texCoords[i];
        output.Color = input[0].Color;
        output.Strength = input[0].Strength;
        stream.Append(output);
    }
}

float4 PS_Trail(GS_OUTPUT_TRAIL input) : SV_TARGET
{
    // Fade towards edges
    float edgeFade = 1.0f - abs(input.TexCoord.y - 0.5f) * 2.0f;
    edgeFade = pow(edgeFade, 0.5f);

    // Animated pulse
    float pulse = sin(Time * 5.0f + input.TexCoord.x * 10.0f) * 0.1f + 0.9f;

    float alpha = edgeFade * input.Strength * pulse;

    return float4(input.Color.rgb * pulse, input.Color.a * alpha);
}

// =============================================================================
// Web Rendering (for spiders)
// =============================================================================

struct VS_INPUT_WEB
{
    float3 Position : POSITION;
    float3 AnchorA : ANCHOR0;
    float3 AnchorB : ANCHOR1;
    float Tension : TENSION;
};

struct PS_INPUT_WEB
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float Tension : TEXCOORD1;
};

PS_INPUT_WEB VS_Web(VS_INPUT_WEB input)
{
    PS_INPUT_WEB output;

    // Catenary curve for web strand
    float t = input.Position.x;  // 0 to 1 along strand

    // Sag based on tension (less tension = more sag)
    float sag = (1.0f - input.Tension) * 0.2f;
    float sagCurve = 4.0f * sag * t * (1.0f - t);

    // Interpolate between anchors with sag
    float3 worldPos = lerp(input.AnchorA, input.AnchorB, t);
    worldPos.y -= sagCurve;

    output.WorldPos = worldPos;
    output.Position = mul(float4(worldPos, 1.0f), ViewProjection);
    output.Tension = input.Tension;

    return output;
}

float4 PS_Web(PS_INPUT_WEB input) : SV_TARGET
{
    // Thin, semi-transparent strand
    float alpha = 0.3f * input.Tension;

    // Slight shimmer
    float shimmer = sin(Time * 2.0f + input.WorldPos.x * 50.0f) * 0.1f + 0.9f;

    // Silver-white color
    float3 color = float3(0.9f, 0.9f, 0.95f) * shimmer;

    return float4(color, alpha);
}

// =============================================================================
// Butterfly Wing Rendering
// =============================================================================

struct VS_INPUT_WING
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float WingAngle : WING_ANGLE;
};

struct PS_INPUT_WING
{
    float4 Position : SV_POSITION;
    float3 Normal : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
};

Texture2D WingPattern : register(t0);
SamplerState WingSampler : register(s0);

PS_INPUT_WING VS_Wing(VS_INPUT_WING input, float4x4 World : INST_WORLD, float4 Color : INST_COLOR, float4 Params : INST_PARAMS)
{
    PS_INPUT_WING output;

    // Animate wing flap
    float flapAngle = sin(Params.y * 8.0f) * 0.5f;  // Flapping animation

    // Rotate wing around body axis (X)
    float cosFlap = cos(input.WingAngle + flapAngle);
    float sinFlap = sin(input.WingAngle + flapAngle);

    float3 localPos = input.Position;
    float y = localPos.y * cosFlap - localPos.z * sinFlap;
    float z = localPos.y * sinFlap + localPos.z * cosFlap;
    localPos.y = y;
    localPos.z = z;

    // Transform to world
    float4 worldPos = mul(float4(localPos, 1.0f), World);
    output.WorldPos = worldPos.xyz;
    output.Position = mul(worldPos, ViewProjection);

    // Transform normal
    float3 localNormal = input.Normal;
    float ny = localNormal.y * cosFlap - localNormal.z * sinFlap;
    float nz = localNormal.y * sinFlap + localNormal.z * cosFlap;
    localNormal.y = ny;
    localNormal.z = nz;

    float3x3 normalMatrix = (float3x3)World;
    output.Normal = normalize(mul(localNormal, normalMatrix));

    output.TexCoord = input.TexCoord;

    return output;
}

float4 PS_Wing(PS_INPUT_WING input) : SV_TARGET
{
    // Sample wing pattern (or use procedural)
    float4 pattern = WingPattern.Sample(WingSampler, input.TexCoord);

    // Basic lighting
    float3 N = normalize(input.Normal);
    float NdotL = max(0.0f, dot(N, -SunDirection));

    // Translucent wings - light from behind
    float backlight = max(0.0f, dot(-N, -SunDirection)) * 0.3f;

    float3 diffuse = pattern.rgb * (AmbientColor.rgb * 0.5f + SunColor.rgb * (NdotL + backlight));

    // Iridescence effect
    float3 viewDir = normalize(CameraPosition - input.WorldPos);
    float fresnel = 1.0f - abs(dot(N, viewDir));
    fresnel = pow(fresnel, 2.0f);

    // Shift hue based on view angle
    float3 iridescent = float3(
        sin(fresnel * 3.14f * 2.0f) * 0.5f + 0.5f,
        sin(fresnel * 3.14f * 2.0f + 2.094f) * 0.5f + 0.5f,
        sin(fresnel * 3.14f * 2.0f + 4.188f) * 0.5f + 0.5f
    );

    diffuse += iridescent * fresnel * 0.3f;

    return float4(diffuse, pattern.a);
}

// =============================================================================
// Techniques
// =============================================================================

/*
technique11 InstancedMesh
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Instanced()));
        SetPixelShader(CompileShader(ps_5_0, PS_Instanced()));
    }
}

technique11 PointSprites
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Point()));
        SetGeometryShader(CompileShader(gs_5_0, GS_Point()));
        SetPixelShader(CompileShader(ps_5_0, PS_Point()));
    }
}

technique11 Particles
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Particle()));
        SetGeometryShader(CompileShader(gs_5_0, GS_Particle()));
        SetPixelShader(CompileShader(ps_5_0, PS_Particle()));
    }
}

technique11 PheromoneTrails
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Trail()));
        SetGeometryShader(CompileShader(gs_5_0, GS_Trail()));
        SetPixelShader(CompileShader(ps_5_0, PS_Trail()));
    }
}

technique11 SpiderWebs
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Web()));
        SetPixelShader(CompileShader(ps_5_0, PS_Web()));
    }
}

technique11 ButterflyWings
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Wing()));
        SetPixelShader(CompileShader(ps_5_0, PS_Wing()));
    }
}
*/
