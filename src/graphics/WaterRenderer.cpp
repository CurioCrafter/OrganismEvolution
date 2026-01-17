#include "WaterRenderer.h"
#include <iostream>
#include <cstring>

using namespace Forge;
using namespace Forge::RHI;

// Embedded HLSL shader source for water rendering
static const char* WATER_SHADER_SOURCE = R"HLSL(
// Water Shader - Animated waves with fresnel reflections

cbuffer WaterConstants : register(b0) {
    // View/Projection matrices
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;

    // Camera and lighting
    float3 cameraPos;
    float time;
    float3 lightDir;
    float _pad1;
    float3 lightColor;
    float sunIntensity;

    // Water colors
    float4 waterColor;      // Deep water
    float4 shallowColor;    // Shallow water

    // Wave parameters
    float waveScale;
    float waveSpeed;
    float waveHeight;
    float transparency;

    // Sky colors for reflection
    float3 skyTopColor;
    float _pad2;
    float3 skyHorizonColor;
    float fresnelPower;

    // Foam and specular
    float foamIntensity;
    float foamScale;
    float specularPower;
    float specularIntensity;

    // Underwater rendering
    float waterHeight;        // Y-level of water surface
    float underwaterDepth;    // Camera depth below surface (0 if above)
    float surfaceClarity;     // How clear the surface is from below
    float _pad3;
};

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 viewDir  : TEXCOORD2;
};

// Simple hash for noise
float hash(float2 p) {
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// Value noise
float noise(float2 p) {
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + float2(1, 0));
    float c = hash(i + float2(0, 1));
    float d = hash(i + float2(1, 1));

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

// FBM noise for wave detail
float fbm(float2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// Calculate wave displacement and normal
float3 calculateWaveOffset(float3 worldPos, out float3 normal) {
    float2 uv = worldPos.xz;

    // Multi-layer wave animation
    float2 wave1UV = uv * waveScale * 0.5 + float2(time * waveSpeed, time * waveSpeed * 0.7);
    float2 wave2UV = uv * waveScale * 1.0 + float2(-time * waveSpeed * 0.8, time * waveSpeed * 0.5);
    float2 wave3UV = uv * waveScale * 2.0 + float2(time * waveSpeed * 0.3, -time * waveSpeed * 0.4);

    // Wave heights
    float wave1 = sin(wave1UV.x * 3.14159 + wave1UV.y * 2.5) * 0.5 + 0.5;
    float wave2 = sin(wave2UV.x * 2.0 - wave2UV.y * 3.0) * 0.5 + 0.5;
    float wave3 = fbm(wave3UV, 3) * 0.3;

    float combinedHeight = (wave1 * 0.5 + wave2 * 0.3 + wave3) * waveHeight;

    // Calculate normal from partial derivatives
    float epsilon = 0.1;
    float hL = sin((uv.x - epsilon) * waveScale * 0.5 * 3.14159 + (uv.y) * waveScale * 0.5 * 2.5);
    float hR = sin((uv.x + epsilon) * waveScale * 0.5 * 3.14159 + (uv.y) * waveScale * 0.5 * 2.5);
    float hD = sin((uv.x) * waveScale * 0.5 * 3.14159 + (uv.y - epsilon) * waveScale * 0.5 * 2.5);
    float hU = sin((uv.x) * waveScale * 0.5 * 3.14159 + (uv.y + epsilon) * waveScale * 0.5 * 2.5);

    float3 tangentX = normalize(float3(epsilon * 2, (hR - hL) * waveHeight, 0));
    float3 tangentZ = normalize(float3(0, (hU - hD) * waveHeight, epsilon * 2));
    normal = normalize(cross(tangentZ, tangentX));

    return float3(0, combinedHeight, 0);
}

PSInput VSMain(VSInput input) {
    PSInput output;

    float3 worldPos = input.position;

    // Calculate wave displacement
    float3 waveNormal;
    float3 waveOffset = calculateWaveOffset(worldPos, waveNormal);
    worldPos += waveOffset;

    // Transform to clip space
    output.position = mul(float4(worldPos, 1.0), viewProjection);
    output.worldPos = worldPos;
    output.texCoord = input.texCoord;
    output.viewDir = normalize(cameraPos - worldPos);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 worldPos = input.worldPos;
    float3 viewDir = normalize(input.viewDir);

    // Recalculate wave normal in pixel shader for better quality
    float3 waveNormal;
    calculateWaveOffset(worldPos, waveNormal);

    // Detect if camera is underwater (looking up at surface)
    bool cameraUnderwater = underwaterDepth > 0.0;

    // Flip normal for underwater viewing (surface is above us)
    float3 effectiveNormal = cameraUnderwater ? -waveNormal : waveNormal;

    // Fresnel effect - more reflection at grazing angles
    float fresnel = pow(1.0 - max(dot(viewDir, effectiveNormal), 0.0), fresnelPower);
    fresnel = saturate(fresnel);

    float3 finalColor;
    float alpha;

    if (cameraUnderwater)
    {
        // ========================================
        // UNDERWATER VIEW (looking up at surface)
        // ========================================

        // When looking up at surface from below, we see:
        // 1. Total internal reflection at steep angles (mirror-like)
        // 2. Refracted sky/above-water scene at shallow angles
        // 3. Snell's window effect

        // Critical angle for water (~48.6 degrees from normal)
        float criticalAngleCos = 0.66;  // cos(48.6 degrees)
        float viewDotUp = dot(viewDir, float3(0, 1, 0));

        // Total internal reflection outside Snell's window
        float tirFactor = saturate((criticalAngleCos - abs(viewDotUp)) / 0.2);

        // Inside Snell's window - clearer view of above
        float snellWindow = 1.0 - tirFactor;

        // Underwater surface color (dark blue-green from below)
        float3 underwaterSurfaceColor = float3(0.05, 0.15, 0.25);

        // Sky seen through Snell's window (brighter, with refraction)
        float3 refractedSky = lerp(skyHorizonColor, skyTopColor, saturate(viewDotUp * 2.0));
        refractedSky *= 0.7;  // Attenuated by water

        // Blend based on viewing angle
        finalColor = lerp(underwaterSurfaceColor, refractedSky, snellWindow * surfaceClarity);

        // Add subtle caustic shimmer on the surface
        float2 uv = worldPos.xz;
        float2 causticUV = uv * waveScale * 2.0 + time * 0.3;
        float caustic = sin(causticUV.x * 6.28) * sin(causticUV.y * 6.28);
        caustic = caustic * 0.5 + 0.5;
        caustic = pow(caustic, 3.0);
        finalColor += caustic * 0.1 * snellWindow;

        // Reduce foam visibility from below
        float foam = 0.0;

        // Transparency: more opaque from below (we see the surface clearly)
        alpha = lerp(0.85, 0.95, snellWindow);

        // Slight wave distortion on surface brightness
        float wavePeak = sin(uv.x * waveScale * 3.14159 + uv.y * waveScale * 2.5 + time * waveSpeed) * 0.5 + 0.5;
        finalColor += wavePeak * 0.02;
    }
    else
    {
        // ========================================
        // ABOVE WATER VIEW (original behavior)
        // ========================================

        // Sky reflection color (simple gradient)
        float skyMix = saturate(waveNormal.y);
        float3 skyReflection = lerp(skyHorizonColor, skyTopColor, skyMix);

        // Sun specular highlight
        float3 halfVec = normalize(lightDir + viewDir);
        float spec = pow(max(dot(waveNormal, halfVec), 0.0), specularPower);
        float3 specular = lightColor * spec * specularIntensity * sunIntensity;

        // Wave highlight based on wave peaks
        float2 uv = worldPos.xz;
        float2 waveUV = uv * waveScale + float2(time * waveSpeed, time * waveSpeed * 0.7);
        float wavePeak = sin(waveUV.x * 3.14159 + waveUV.y * 2.5) * 0.5 + 0.5;
        wavePeak = pow(wavePeak, 4.0);

        // Foam on wave peaks
        float foam = wavePeak * foamIntensity;
        float foamNoise = fbm(uv * foamScale + time * 0.5, 2);
        foam *= foamNoise;

        // Blend water color with sky reflection based on fresnel
        float3 baseWater = lerp(waterColor.rgb, shallowColor.rgb, 0.3);
        finalColor = lerp(baseWater, skyReflection, fresnel * 0.6);

        // Add specular highlights
        finalColor += specular;

        // Add foam
        finalColor = lerp(finalColor, float3(1, 1, 1), saturate(foam));

        // Add subtle wave highlights
        finalColor += wavePeak * 0.05 * lightColor;

        // Apply transparency
        alpha = transparency;

        // Less transparent at grazing angles (more reflection = more opaque looking)
        alpha = lerp(alpha, 1.0, fresnel * 0.3);
    }

    return float4(finalColor, alpha);
}
)HLSL";

WaterRenderer::WaterRenderer() = default;

WaterRenderer::~WaterRenderer() {
    Shutdown();
}

bool WaterRenderer::Initialize(IDevice* device,
                               Format renderTargetFormat,
                               Format depthFormat) {
    if (m_initialized) {
        return true;
    }

    m_device = device;

    if (!CreateShaders()) {
        std::cerr << "[WaterRenderer] Failed to create shaders" << std::endl;
        return false;
    }

    if (!CreatePipeline(renderTargetFormat, depthFormat)) {
        std::cerr << "[WaterRenderer] Failed to create pipeline" << std::endl;
        return false;
    }

    if (!CreateBuffers()) {
        std::cerr << "[WaterRenderer] Failed to create buffers" << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "[WaterRenderer] Initialized successfully" << std::endl;
    return true;
}

void WaterRenderer::Shutdown() {
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_constantBuffer.reset();
    m_pipeline.reset();
    m_vertexShader.reset();
    m_pixelShader.reset();
    m_vertices.clear();
    m_indices.clear();
    m_initialized = false;
}

bool WaterRenderer::CreateShaders() {
    // Create vertex shader
    ShaderDesc vsDesc;
    vsDesc.type = ShaderType::Vertex;
    vsDesc.source = WATER_SHADER_SOURCE;
    vsDesc.entryPoint = "VSMain";
    vsDesc.debugName = "WaterVS";

    m_vertexShader = m_device->CreateShader(vsDesc);
    if (!m_vertexShader) {
        std::cerr << "[WaterRenderer] Failed to create vertex shader" << std::endl;
        return false;
    }

    // Create pixel shader
    ShaderDesc psDesc;
    psDesc.type = ShaderType::Pixel;
    psDesc.source = WATER_SHADER_SOURCE;
    psDesc.entryPoint = "PSMain";
    psDesc.debugName = "WaterPS";

    m_pixelShader = m_device->CreateShader(psDesc);
    if (!m_pixelShader) {
        std::cerr << "[WaterRenderer] Failed to create pixel shader" << std::endl;
        return false;
    }

    return true;
}

bool WaterRenderer::CreatePipeline(Format renderTargetFormat, Format depthFormat) {
    PipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_vertexShader.get();
    pipelineDesc.pixelShader = m_pixelShader.get();

    // Vertex layout
    pipelineDesc.vertexLayout = {
        { "POSITION", 0, Format::R32G32B32_FLOAT, 0, 0, InputRate::PerVertex, 0 },
        { "TEXCOORD", 0, Format::R32G32_FLOAT, 0, 12, InputRate::PerVertex, 0 }
    };

    pipelineDesc.primitiveTopology = PrimitiveTopology::TriangleList;

    // Rasterizer state
    pipelineDesc.fillMode = FillMode::Solid;
    pipelineDesc.cullMode = CullMode::None;  // Render both sides of water
    pipelineDesc.frontFace = FrontFace::CounterClockwise;
    pipelineDesc.depthClipEnabled = true;

    // Depth state
    pipelineDesc.depthTestEnabled = true;
    pipelineDesc.depthWriteEnabled = false;  // Don't write to depth for transparency
    pipelineDesc.depthCompareOp = CompareOp::Less;

    // Blend state for transparency
    pipelineDesc.blendEnabled = true;
    pipelineDesc.srcBlend = BlendFactor::SrcAlpha;
    pipelineDesc.dstBlend = BlendFactor::InvSrcAlpha;
    pipelineDesc.blendOp = BlendOp::Add;

    // Render target format
    pipelineDesc.renderTargetFormats.push_back(renderTargetFormat);
    pipelineDesc.depthStencilFormat = depthFormat;

    pipelineDesc.debugName = "WaterPipeline";

    m_pipeline = m_device->CreatePipeline(pipelineDesc);
    if (!m_pipeline) {
        std::cerr << "[WaterRenderer] Failed to create pipeline" << std::endl;
        return false;
    }

    return true;
}

bool WaterRenderer::CreateBuffers() {
    // Create constant buffer (will be updated each frame)
    BufferDesc cbDesc;
    cbDesc.size = sizeof(WaterConstants) * 2;  // Double-buffered
    cbDesc.usage = BufferUsage::Uniform;
    cbDesc.cpuAccess = true;
    cbDesc.debugName = "WaterConstantBuffer";

    m_constantBuffer = m_device->CreateBuffer(cbDesc);
    if (!m_constantBuffer) {
        std::cerr << "[WaterRenderer] Failed to create constant buffer" << std::endl;
        return false;
    }

    return true;
}

void WaterRenderer::GenerateMesh(int gridSize, float worldSize, float waterHeight) {
    m_waterHeight = waterHeight;
    m_vertices.clear();
    m_indices.clear();

    float halfSize = worldSize * 0.5f;
    float step = worldSize / static_cast<float>(gridSize - 1);

    // Generate vertices
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            WaterVertex vertex;
            vertex.position = glm::vec3(
                -halfSize + x * step,
                waterHeight,
                -halfSize + z * step
            );
            vertex.texCoord = glm::vec2(
                static_cast<float>(x) / static_cast<float>(gridSize - 1),
                static_cast<float>(z) / static_cast<float>(gridSize - 1)
            );
            m_vertices.push_back(vertex);
        }
    }

    // Generate indices
    for (int z = 0; z < gridSize - 1; ++z) {
        for (int x = 0; x < gridSize - 1; ++x) {
            uint32_t topLeft = z * gridSize + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * gridSize + x;
            uint32_t bottomRight = bottomLeft + 1;

            // First triangle
            m_indices.push_back(topLeft);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(topRight);

            // Second triangle
            m_indices.push_back(topRight);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(bottomRight);
        }
    }

    m_vertexCount = static_cast<int>(m_vertices.size());
    m_indexCount = static_cast<int>(m_indices.size());

    // Create vertex buffer
    BufferDesc vbDesc;
    vbDesc.size = m_vertices.size() * sizeof(WaterVertex);
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuAccess = true;
    vbDesc.debugName = "WaterVertexBuffer";

    m_vertexBuffer = m_device->CreateBuffer(vbDesc);
    if (m_vertexBuffer) {
        void* data = m_vertexBuffer->Map();
        if (data) {
            memcpy(data, m_vertices.data(), vbDesc.size);
            m_vertexBuffer->Unmap();
        }
    }

    // Create index buffer
    BufferDesc ibDesc;
    ibDesc.size = m_indices.size() * sizeof(uint32_t);
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuAccess = true;
    ibDesc.debugName = "WaterIndexBuffer";

    m_indexBuffer = m_device->CreateBuffer(ibDesc);
    if (m_indexBuffer) {
        void* data = m_indexBuffer->Map();
        if (data) {
            memcpy(data, m_indices.data(), ibDesc.size);
            m_indexBuffer->Unmap();
        }
    }

    std::cout << "[WaterRenderer] Generated mesh: " << m_vertexCount << " vertices, "
              << m_indexCount << " indices" << std::endl;
}

void WaterRenderer::SetWaterColor(const glm::vec4& deepColor, const glm::vec4& shallowColor) {
    m_deepWaterColor = deepColor;
    m_shallowWaterColor = shallowColor;
}

void WaterRenderer::SetWaveParams(float scale, float speed, float height) {
    m_waveScale = scale;
    m_waveSpeed = speed;
    m_waveHeight = height;
}

void WaterRenderer::SetFoamParams(float intensity, float scale) {
    m_foamIntensity = intensity;
    m_foamScale = scale;
}

void WaterRenderer::SetSpecularParams(float power, float intensity) {
    m_specularPower = power;
    m_specularIntensity = intensity;
}

void WaterRenderer::SetSkyColors(const glm::vec3& topColor, const glm::vec3& horizonColor) {
    m_skyTopColor = topColor;
    m_skyHorizonColor = horizonColor;
}

float WaterRenderer::GetUnderwaterDepth(const glm::vec3& cameraPos) const {
    // Return how deep the camera is below the water surface
    // Positive values = underwater, negative = above water
    return m_waterHeight - cameraPos.y;
}

void WaterRenderer::UpdateConstantBuffer(const glm::mat4& view,
                                         const glm::mat4& projection,
                                         const glm::vec3& cameraPos,
                                         const glm::vec3& lightDir,
                                         const glm::vec3& lightColor,
                                         float sunIntensity,
                                         float time) {
    WaterConstants constants = {};

    // Matrices
    constants.view = view;
    constants.projection = projection;
    constants.viewProjection = projection * view;

    // Camera and lighting
    constants.cameraPos = cameraPos;
    constants.time = time;
    constants.lightDir = glm::normalize(lightDir);
    constants.lightColor = lightColor;
    constants.sunIntensity = sunIntensity;

    // Water colors
    constants.waterColor = m_deepWaterColor;
    constants.shallowColor = m_shallowWaterColor;

    // Wave parameters
    constants.waveScale = m_waveScale;
    constants.waveSpeed = m_waveSpeed;
    constants.waveHeight = m_waveHeight;
    constants.transparency = m_transparency;

    // Sky colors
    constants.skyTopColor = m_skyTopColor;
    constants.skyHorizonColor = m_skyHorizonColor;
    constants.fresnelPower = m_fresnelPower;

    // Foam and specular
    constants.foamIntensity = m_foamIntensity;
    constants.foamScale = m_foamScale;
    constants.specularPower = m_specularPower;
    constants.specularIntensity = m_specularIntensity;

    // Underwater parameters
    constants.waterHeight = m_waterHeight;
    float depth = GetUnderwaterDepth(cameraPos);
    constants.underwaterDepth = depth > 0.0f ? depth : 0.0f;  // Only positive when underwater
    constants.surfaceClarity = m_underwaterParams.clarityScalar;

    // Update buffer
    void* data = m_constantBuffer->Map();
    if (data) {
        memcpy(data, &constants, sizeof(WaterConstants));
        m_constantBuffer->Unmap();
    }
}

void WaterRenderer::Render(ICommandList* commandList,
                           const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::vec3& cameraPos,
                           const glm::vec3& lightDir,
                           const glm::vec3& lightColor,
                           float sunIntensity,
                           float time) {
    if (!m_initialized || !m_vertexBuffer || !m_indexBuffer || m_indexCount == 0) {
        return;
    }

    // Update constant buffer with current frame data
    UpdateConstantBuffer(view, projection, cameraPos, lightDir, lightColor, sunIntensity, time);

    // Set pipeline state
    commandList->SetPipeline(m_pipeline.get());

    // Bind buffers
    commandList->BindVertexBuffer(0, m_vertexBuffer.get(), sizeof(WaterVertex), 0);
    commandList->BindIndexBuffer(m_indexBuffer.get(), IndexFormat::UInt32, 0);
    commandList->BindConstantBuffer(0, m_constantBuffer.get(), 0);

    // Draw
    commandList->DrawIndexed(m_indexCount, 0, 0);
}
