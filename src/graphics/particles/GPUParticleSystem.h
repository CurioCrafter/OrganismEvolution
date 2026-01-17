// GPUParticleSystem.h - GPU-based particle system for weather effects
// Uses compute shaders for particle simulation and instanced rendering
#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include "../DX12Device.h"

using Microsoft::WRL::ComPtr;

// Forward declarations
class Camera;
class Terrain;

// Maximum particles - hard cap to prevent buffer overflow
static constexpr uint32_t MAX_WEATHER_PARTICLES = 50000;

// Particle structure - matches HLSL Weather.hlsl struct
struct WeatherParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;         // 0-1, 0 = dead
    float size;
    float rotation;     // For snow tumbling
    float type;         // 0 = rain, 1 = snow
    float padding[2];   // Align to 16 bytes
};
static_assert(sizeof(WeatherParticle) == 48, "WeatherParticle must be 48 bytes");

// Emit parameters for spawning particles
struct ParticleEmitParams {
    glm::vec3 position;
    glm::vec3 velocity;
    float life = 1.0f;
    float size = 0.02f;
    float rotation = 0.0f;
    float type = 0.0f;  // 0 = rain, 1 = snow
};

// Weather constant buffer - matches Weather.hlsl cbuffer
// Must be exactly 256 bytes for DX12 constant buffer alignment
struct WeatherConstantBuffer {
    glm::mat4 viewProjection;           // 64 bytes (offset 0)
    glm::vec4 cameraPos;                // 16 bytes (offset 64) - w = time
    glm::vec4 windParams;               // 16 bytes (offset 80) - xyz = direction, w = strength
    glm::vec4 weatherParams;            // 16 bytes (offset 96) - x = precipIntensity, y = precipType, z = fogDensity, w = fogHeight
    glm::vec4 boundsMin;                // 16 bytes (offset 112) - xyz = min, w = spawnHeight
    glm::vec4 boundsMax;                // 16 bytes (offset 128) - xyz = max, w = groundLevel
    glm::vec4 rainColor;                // 16 bytes (offset 144) - xyz = color, w = alpha
    glm::vec4 snowColor;                // 16 bytes (offset 160) - xyz = color, w = alpha
    glm::vec4 lightningParams;          // 16 bytes (offset 176) - x = intensity, yzw = position
    float padding[16];                  // 64 bytes (offset 192) - pad to 256
};
static_assert(sizeof(WeatherConstantBuffer) == 256, "WeatherConstantBuffer must be 256 bytes");

// GPU Particle System class
class GPUParticleSystem {
public:
    GPUParticleSystem();
    ~GPUParticleSystem();

    // Non-copyable
    GPUParticleSystem(const GPUParticleSystem&) = delete;
    GPUParticleSystem& operator=(const GPUParticleSystem&) = delete;

    // Initialize DX12 resources
    bool initialize(DX12Device* device);
    void shutdown();

    // Emit particles (CPU-side, batched for next update)
    void emit(const ParticleEmitParams& params);
    void emitBurst(const std::vector<ParticleEmitParams>& particles);

    // Clear all particles
    void clearAll();

    // Update particle simulation (runs compute shader)
    void update(float deltaTime,
                const glm::vec3& windDirection,
                float windStrength,
                float precipitationIntensity,
                float precipitationType,
                float fogDensity,
                float groundLevel,
                const glm::vec3& cameraPos);

    // Render particles
    void render(ID3D12GraphicsCommandList* cmdList,
                const Camera& camera,
                const glm::mat4& viewProjection,
                float time,
                float lightningIntensity = 0.0f,
                const glm::vec3& lightningPos = glm::vec3(0.0f));

    // Get current particle count (approximate - GPU-side value may differ)
    uint32_t getApproximateParticleCount() const { return m_cpuParticleCount; }

    // Set terrain reference for ground collision
    void setTerrain(const Terrain* terrain) { m_terrain = terrain; }

    // Configuration
    void setRainColor(const glm::vec3& color, float alpha);
    void setSnowColor(const glm::vec3& color, float alpha);
    void setSpawnBounds(const glm::vec3& min, const glm::vec3& max, float spawnHeight);

    bool isInitialized() const { return m_initialized; }

private:
    bool createBuffers();
    bool createComputePipeline();
    bool createRenderPipeline();
    bool createBillboardMesh();
    void uploadPendingParticles();
    void updateConstantBuffer(const Camera& camera,
                              const glm::mat4& viewProjection,
                              float time,
                              float lightningIntensity,
                              const glm::vec3& lightningPos);

    DX12Device* m_device = nullptr;
    const Terrain* m_terrain = nullptr;

    // Particle buffers (double-buffered for ping-pong simulation)
    ComPtr<ID3D12Resource> m_particleBuffers[2];
    ComPtr<ID3D12Resource> m_particleUploadBuffer;
    ComPtr<ID3D12Resource> m_aliveCountBuffer;
    ComPtr<ID3D12Resource> m_aliveCountReadback;

    // Constant buffer
    ComPtr<ID3D12Resource> m_constantBuffer;
    WeatherConstantBuffer* m_mappedConstantBuffer = nullptr;

    // Billboard mesh for instanced rendering
    ComPtr<ID3D12Resource> m_billboardVertexBuffer;
    ComPtr<ID3D12Resource> m_billboardIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_billboardVBView = {};
    D3D12_INDEX_BUFFER_VIEW m_billboardIBView = {};

    // Compute pipeline for particle simulation
    ComPtr<ID3D12PipelineState> m_computePSO;
    ComPtr<ID3D12RootSignature> m_computeRootSig;

    // Render pipeline for particle drawing
    ComPtr<ID3D12PipelineState> m_renderPSO;
    ComPtr<ID3D12RootSignature> m_renderRootSig;

    // Descriptor heap indices
    UINT m_particleBufferSRVIndex[2] = {0, 0};
    UINT m_particleBufferUAVIndex[2] = {0, 0};
    UINT m_aliveCountUAVIndex = 0;
    UINT m_constantBufferCBVIndex = 0;

    // State tracking
    int m_currentBufferIndex = 0;
    uint32_t m_cpuParticleCount = 0;

    // Pending particles to upload
    std::vector<WeatherParticle> m_pendingParticles;

    // Visual settings
    glm::vec3 m_rainColor = glm::vec3(0.7f, 0.75f, 0.85f);
    float m_rainAlpha = 0.3f;
    glm::vec3 m_snowColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float m_snowAlpha = 0.8f;

    // Spawn bounds
    glm::vec3 m_boundsMin = glm::vec3(-100.0f, 0.0f, -100.0f);
    glm::vec3 m_boundsMax = glm::vec3(100.0f, 100.0f, 100.0f);
    float m_spawnHeight = 50.0f;

    // Weather state (updated each frame)
    glm::vec3 m_windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    float m_windStrength = 0.0f;
    float m_precipitationIntensity = 0.0f;
    float m_precipitationType = 0.0f;
    float m_fogDensity = 0.0f;
    float m_groundLevel = 0.0f;

    bool m_initialized = false;
};
