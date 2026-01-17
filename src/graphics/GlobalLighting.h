#pragma once

// GlobalLighting.h - Centralized global lighting management
// Ensures consistent light direction across all render systems (terrain, creatures, shadows, water)
// This is the AUTHORITATIVE source for directional light - all systems should query this

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#endif

#include <DirectXMath.h>
#include "../core/DayNightCycle.h"

// Forward declarations
class ShadowMap_DX12;
class PlanetTheme;

// ============================================================================
// Global Lighting Constants (must match Common.hlsl cbuffer)
// ============================================================================
struct alignas(256) GlobalLightingConstants
{
    // Primary directional light (sun or moon)
    DirectX::XMFLOAT3 lightDirection;        // 0-11
    float lightIntensity;                     // 12-15
    DirectX::XMFLOAT3 lightColor;            // 16-27
    float shadowStrength;                     // 28-31

    // Ambient lighting
    DirectX::XMFLOAT3 ambientColorSky;       // 32-43 (from above)
    float ambientSkyStrength;                // 44-47
    DirectX::XMFLOAT3 ambientColorGround;    // 48-59 (from below)
    float ambientGroundStrength;             // 60-63

    // Secondary light (moon when sun is primary, or vice versa)
    DirectX::XMFLOAT3 secondaryLightDir;     // 64-75
    float secondaryIntensity;                 // 76-79
    DirectX::XMFLOAT3 secondaryLightColor;   // 80-91
    float padding1;                           // 92-95

    // Environment
    DirectX::XMFLOAT3 fogColor;              // 96-107
    float fogDensity;                         // 108-111
    float fogStart;                           // 112-115
    float fogEnd;                             // 116-119
    float timeOfDay;                          // 120-123
    float sunElevation;                       // 124-127

    // Water level for underwater lighting
    float waterLevel;                         // 128-131
    float underwaterFogDensity;              // 132-135
    float underwaterLightFalloff;            // 136-139
    float moonPhase;                          // 140-143

    // Cascade shadow map info (for shader)
    DirectX::XMFLOAT4 cascadeSplits;         // 144-159
    DirectX::XMFLOAT4X4 lightViewProj[4];    // 160-415 (4 cascades)

    // Padding to 512 bytes
    float padding2[24];                       // 416-511
};

static_assert(sizeof(GlobalLightingConstants) == 512, "GlobalLightingConstants must be 512 bytes");

// ============================================================================
// Visual Style Parameters (No Man's Sky inspired)
// Central configuration for stylized rendering across all systems
// ============================================================================
struct VisualStyleParams
{
    // Master controls
    float styleStrength = 0.7f;              // 0-1: blend between realistic and stylized
    float colorVibrancy = 1.15f;             // Overall saturation boost (1.0 = neutral)

    // Creature styling
    float creatureRimIntensity = 0.45f;      // Stylized rim lighting intensity
    float creatureRimPower = 2.5f;           // Rim falloff (lower = broader)
    float creatureGradientStrength = 0.3f;   // Vertical gradient shading
    float creatureColorBoost = 1.15f;        // Creature saturation

    // Terrain styling
    float terrainStyleStrength = 0.65f;      // Terrain style blend
    float biomeTintStrength = 0.4f;          // How strongly biomes tint terrain
    float slopeVariation = 0.35f;            // Rock exposure on slopes
    float heightGradient = 0.25f;            // Vertical color gradient

    // Vegetation styling
    float vegetationStyleStrength = 0.7f;    // Vegetation style blend
    float foliageSheen = 0.15f;              // Waxy leaf shine
    float vegetationVibrancy = 1.18f;        // Plant saturation
    float vegetationRimIntensity = 0.3f;     // Edge lighting on leaves

    // Sky and atmosphere
    float sunGlowIntensity = 0.3f;           // Stylized sun glow
    float horizonGlow = 0.25f;               // Horizon atmospheric glow
    float skyGradientContrast = 1.1f;        // Sky color contrast

    // Color grading (subtle)
    DirectX::XMFLOAT3 colorFilter = { 1.0f, 1.0f, 1.0f };
    float shadowWarmth = 0.1f;               // Warm shadows
    float highlightCool = 0.05f;             // Cool highlights

    // Theme tints (for biome-specific looks)
    DirectX::XMFLOAT3 warmBiomeTint = { 1.05f, 1.0f, 0.92f };   // Desert/volcanic
    DirectX::XMFLOAT3 coolBiomeTint = { 0.92f, 0.98f, 1.05f };  // Tundra/alpine
    DirectX::XMFLOAT3 lushBiomeTint = { 0.95f, 1.05f, 0.95f };  // Forest/jungle
    DirectX::XMFLOAT3 alienBiomeTint = { 1.0f, 0.95f, 1.08f };  // Alien purple

    // Get default Earth-like style
    static VisualStyleParams getEarthLike() {
        return VisualStyleParams();  // Default values are Earth-like
    }

    // Get alien world style (more vibrant, purple tints)
    static VisualStyleParams getAlienPurple() {
        VisualStyleParams params;
        params.styleStrength = 0.85f;
        params.colorVibrancy = 1.25f;
        params.colorFilter = { 1.02f, 0.95f, 1.08f };
        params.biomeTintStrength = 0.6f;
        params.horizonGlow = 0.35f;
        return params;
    }

    // Get desert world style (warm, high contrast)
    static VisualStyleParams getDesertWorld() {
        VisualStyleParams params;
        params.styleStrength = 0.75f;
        params.colorFilter = { 1.08f, 1.02f, 0.92f };
        params.shadowWarmth = 0.2f;
        params.sunGlowIntensity = 0.4f;
        params.skyGradientContrast = 1.2f;
        return params;
    }

    // Get frozen world style (cool, high saturation blues)
    static VisualStyleParams getFrozenWorld() {
        VisualStyleParams params;
        params.styleStrength = 0.7f;
        params.colorFilter = { 0.92f, 0.98f, 1.08f };
        params.highlightCool = 0.15f;
        params.shadowWarmth = 0.0f;
        params.colorVibrancy = 1.1f;
        return params;
    }
};

// ============================================================================
// Global Lighting Manager
// ============================================================================
class GlobalLighting
{
public:
    GlobalLighting();
    ~GlobalLighting();

    // Initialize DX12 resources
    bool init(ID3D12Device* device);
    void cleanup();

    // ========================================================================
    // Update from day/night cycle (call each frame before rendering)
    // ========================================================================
    void update(const DayNightCycle& dayNight, const PlanetTheme* theme = nullptr);

    // ========================================================================
    // Configure shadow map to match our light direction
    // CRITICAL: Call this AFTER update() and BEFORE shadow map renders
    // ========================================================================
    void configureShadowMap(ShadowMap_DX12& shadowMap);

    // ========================================================================
    // Upload constants to GPU (call before rendering)
    // ========================================================================
    void uploadConstants(ID3D12GraphicsCommandList* cmdList);

    // ========================================================================
    // Bind to shader (use this for consistent lighting across all systems)
    // ========================================================================
    void bind(ID3D12GraphicsCommandList* cmdList, uint32_t rootParamIndex);
    D3D12_GPU_VIRTUAL_ADDRESS getConstantBufferGPU() const;

    // ========================================================================
    // Query current lighting state (for systems that need it)
    // ========================================================================
    DirectX::XMFLOAT3 getLightDirection() const { return m_constants.lightDirection; }
    DirectX::XMFLOAT3 getLightColor() const { return m_constants.lightColor; }
    float getLightIntensity() const { return m_constants.lightIntensity; }
    DirectX::XMFLOAT3 getAmbientColor() const { return m_constants.ambientColorSky; }
    DirectX::XMFLOAT3 getFogColor() const { return m_constants.fogColor; }
    float getTimeOfDay() const { return m_constants.timeOfDay; }
    bool isNight() const { return m_isNight; }
    float getSunElevation() const { return m_constants.sunElevation; }

    // ========================================================================
    // Set water level (must match Terrain::WATER_LEVEL)
    // ========================================================================
    void setWaterLevel(float level);

    // ========================================================================
    // Override fog settings (for weather effects)
    // ========================================================================
    void setFogOverride(bool enable, float density = 0.0f, const DirectX::XMFLOAT3& color = {});

    // ========================================================================
    // Underwater lighting query
    // ========================================================================
    DirectX::XMFLOAT3 getUnderwaterAdjustedLight(float depth) const;

    const GlobalLightingConstants& getConstants() const { return m_constants; }

    // ========================================================================
    // Visual Style Management
    // ========================================================================
    void setVisualStyle(const VisualStyleParams& params) { m_visualStyle = params; }
    const VisualStyleParams& getVisualStyle() const { return m_visualStyle; }
    VisualStyleParams& getMutableVisualStyle() { return m_visualStyle; }

private:
    void calculateSunDirection(float timeOfDay);
    void calculateMoonDirection(float timeOfDay, float moonPhase);
    void calculateAmbient(float timeOfDay);
    void calculateFog(float timeOfDay);

    // DX12 resources
    ID3D12Device* m_device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferMapped = nullptr;

    // Current state
    GlobalLightingConstants m_constants;
    bool m_isNight = false;
    bool m_fogOverride = false;
    float m_fogOverrideDensity = 0.0f;
    DirectX::XMFLOAT3 m_fogOverrideColor;

    bool m_initialized = false;

    // Visual style parameters
    VisualStyleParams m_visualStyle;
};
