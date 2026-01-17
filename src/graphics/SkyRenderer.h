#pragma once

// SkyRenderer.h - Procedural sky rendering with dynamic sun/moon and stars
// Integrates with DayNightCycle for time-of-day transitions

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif

#include <DirectXMath.h>
#include <cstdint>
#include "../core/DayNightCycle.h"

// Forward declarations
class Camera;
class PlanetTheme;

// ============================================================================
// Sky Constants (must match HLSL cbuffer)
// ============================================================================
struct alignas(256) SkyConstants
{
    // View/Projection
    DirectX::XMFLOAT4X4 invViewProj;         // 0-63

    // Sun parameters
    DirectX::XMFLOAT3 sunDirection;          // 64-75
    float sunIntensity;                       // 76-79
    DirectX::XMFLOAT3 sunColor;              // 80-91
    float sunSize;                            // 92-95

    // Moon parameters
    DirectX::XMFLOAT3 moonDirection;         // 96-107
    float moonPhase;                          // 108-111
    DirectX::XMFLOAT3 moonColor;             // 112-123
    float moonSize;                           // 124-127

    // Sky gradient
    DirectX::XMFLOAT3 zenithColor;           // 128-139
    float starVisibility;                     // 140-143
    DirectX::XMFLOAT3 horizonColor;          // 144-155
    float time;                               // 156-159

    // Atmosphere
    DirectX::XMFLOAT3 fogColor;              // 160-171
    float fogDensity;                         // 172-175

    // Camera
    DirectX::XMFLOAT3 cameraPosition;        // 176-187
    float padding1;                           // 188-191

    // Additional parameters
    float rayleighStrength;                   // 192-195
    float mieStrength;                        // 196-199
    float mieG;                               // 200-203
    float atmosphereHeight;                   // 204-207

    // Padding to 256 bytes
    float padding[12];                        // 208-255
};

static_assert(sizeof(SkyConstants) == 256, "SkyConstants must be 256 bytes for CB alignment");

// ============================================================================
// Sky Renderer Class
// ============================================================================
class SkyRenderer
{
public:
    SkyRenderer();
    ~SkyRenderer();

    // Initialize with DX12 device
    bool init(ID3D12Device* device,
              ID3D12DescriptorHeap* srvHeap,
              uint32_t srvIndex,
              DXGI_FORMAT renderTargetFormat);

    void cleanup();

    // Update sky parameters from day/night cycle
    void update(const DayNightCycle& dayNight, const PlanetTheme* theme = nullptr);

    // Render sky dome (call before opaque geometry)
    void render(ID3D12GraphicsCommandList* cmdList,
                const Camera& camera,
                D3D12_CPU_DESCRIPTOR_HANDLE renderTarget,
                D3D12_CPU_DESCRIPTOR_HANDLE depthStencil);

    // Render sky without depth (for reflection captures)
    void renderForReflection(ID3D12GraphicsCommandList* cmdList,
                              const DirectX::XMMATRIX& view,
                              const DirectX::XMMATRIX& projection);

    // Get current sky colors for other systems (water, fog, etc.)
    DirectX::XMFLOAT3 getZenithColor() const { return m_constants.zenithColor; }
    DirectX::XMFLOAT3 getHorizonColor() const { return m_constants.horizonColor; }
    DirectX::XMFLOAT3 getSunColor() const { return m_constants.sunColor; }
    DirectX::XMFLOAT3 getSunDirection() const { return m_constants.sunDirection; }
    float getSunIntensity() const { return m_constants.sunIntensity; }

    // Configuration
    void setSunSize(float size) { m_sunSize = size; }
    void setMoonSize(float size) { m_moonSize = size; }
    void setAtmosphereParams(float rayleigh, float mie, float mieG);

    // ========================================================================
    // Visual Style Parameters (No Man's Sky inspired)
    // ========================================================================
    struct VisualStyleParams {
        float styleStrength = 0.7f;        // 0-1: blend between realistic and stylized
        float colorVibrancy = 1.15f;       // Overall saturation boost
        float sunGlowIntensity = 0.3f;     // Stylized sun glow
        float horizonGlow = 0.25f;         // Horizon atmospheric glow
        float skyGradientContrast = 1.1f;  // Sky color gradient contrast

        // Theme-specific tints (applied additively)
        DirectX::XMFLOAT3 themeTint = { 0.0f, 0.0f, 0.0f };

        // Color grading
        DirectX::XMFLOAT3 colorFilter = { 1.0f, 1.0f, 1.0f };
        float shadowWarmth = 0.1f;         // Warm shadows
        float highlightCool = 0.05f;       // Cool highlights
    };

    void setVisualStyle(const VisualStyleParams& params) { m_visualStyle = params; }
    const VisualStyleParams& getVisualStyle() const { return m_visualStyle; }

    // Apply color grading to sky output
    void applyColorGrading(DirectX::XMFLOAT3& color) const;

    bool isInitialized() const { return m_initialized; }

private:
    bool createPipelineState(DXGI_FORMAT renderTargetFormat);
    bool createConstantBuffer();
    void updateConstants(const Camera& camera);

    // D3D12 resources
    ID3D12Device* m_device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferMapped = nullptr;

    // Current sky state
    SkyConstants m_constants;

    // Parameters
    float m_sunSize = 0.04f;
    float m_moonSize = 0.03f;
    float m_rayleighStrength = 1.0f;
    float m_mieStrength = 0.003f;
    float m_mieG = 0.76f;
    float m_time = 0.0f;

    // State
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_initialized = false;

    // Visual style parameters
    VisualStyleParams m_visualStyle;
};
