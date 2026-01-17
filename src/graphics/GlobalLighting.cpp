// Include ShadowMap_DX12 BEFORE GlobalLighting to avoid namespace pollution issues
// from DayNightCycle.h's "using namespace" directives
#include "ShadowMap_DX12.h"
#include "GlobalLighting.h"
#include "../environment/PlanetTheme.h"
#include <iostream>
#include <cmath>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Constants for sun position calculation
// Note: PI_F and LOCAL_TWO_PI are defined in DayNightCycle.h
// Use local names to avoid conflict
namespace {
    constexpr float LOCAL_PI = 3.14159265359f;
    constexpr float LOCAL_TWO_PI = 6.28318530718f;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

GlobalLighting::GlobalLighting()
{
    memset(&m_constants, 0, sizeof(m_constants));

    // Default values
    m_constants.lightDirection = XMFLOAT3(0.0f, -1.0f, 0.0f);
    m_constants.lightColor = XMFLOAT3(1.0f, 0.98f, 0.95f);
    m_constants.lightIntensity = 1.0f;
    m_constants.shadowStrength = 0.7f;

    m_constants.ambientColorSky = XMFLOAT3(0.3f, 0.35f, 0.4f);
    m_constants.ambientSkyStrength = 0.3f;
    m_constants.ambientColorGround = XMFLOAT3(0.15f, 0.12f, 0.1f);
    m_constants.ambientGroundStrength = 0.1f;

    m_constants.fogColor = XMFLOAT3(0.6f, 0.65f, 0.75f);
    m_constants.fogDensity = 0.0003f;
    m_constants.fogStart = 50.0f;
    m_constants.fogEnd = 1000.0f;

    m_constants.waterLevel = 0.0f;
    m_constants.underwaterFogDensity = 0.05f;
    m_constants.underwaterLightFalloff = 0.02f;

    m_constants.cascadeSplits = XMFLOAT4(0.1f, 0.3f, 0.6f, 1.0f);
}

GlobalLighting::~GlobalLighting()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool GlobalLighting::init(ID3D12Device* device)
{
    if (!device) {
        std::cerr << "[GlobalLighting] Invalid device!" << std::endl;
        return false;
    }

    m_device = device;

    // Create upload heap constant buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = sizeof(GlobalLightingConstants);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)
    );

    if (FAILED(hr)) {
        std::cerr << "[GlobalLighting] Failed to create constant buffer" << std::endl;
        return false;
    }

    m_constantBuffer->SetName(L"GlobalLighting_ConstantBuffer");

    // Map for CPU writes
    D3D12_RANGE readRange = {0, 0};
    hr = m_constantBuffer->Map(0, &readRange, &m_constantBufferMapped);
    if (FAILED(hr)) {
        std::cerr << "[GlobalLighting] Failed to map constant buffer" << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "[GlobalLighting] Initialized successfully" << std::endl;
    return true;
}

void GlobalLighting::cleanup()
{
    if (m_constantBuffer && m_constantBufferMapped) {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferMapped = nullptr;
    }

    m_constantBuffer.Reset();
    m_device = nullptr;
    m_initialized = false;
}

// ============================================================================
// Update from Day/Night Cycle
// ============================================================================

void GlobalLighting::update(const DayNightCycle& dayNight, const PlanetTheme* theme)
{
    float timeOfDay = dayNight.GetTimeOfDay();
    m_constants.timeOfDay = timeOfDay;
    m_isNight = dayNight.IsNight();

    // Calculate sun direction
    calculateSunDirection(timeOfDay);

    // Calculate moon direction
    calculateMoonDirection(timeOfDay, dayNight.GetMoonPhase());
    m_constants.moonPhase = dayNight.GetMoonPhase();

    // Get colors from day/night cycle
    SkyColors colors = dayNight.GetSkyColors();

    // Primary light (sun during day, moon at night)
    if (!m_isNight) {
        // Daytime - sun is primary
        m_constants.lightColor = XMFLOAT3(colors.sunColor.x, colors.sunColor.y, colors.sunColor.z);
        m_constants.lightIntensity = colors.sunIntensity;
        m_constants.shadowStrength = 0.7f * colors.sunIntensity;
    } else {
        // Nighttime - moon is primary (if visible)
        float moonIntensity = dayNight.GetMoonIntensity();
        m_constants.lightDirection = m_constants.secondaryLightDir;  // Use moon direction
        m_constants.lightColor = XMFLOAT3(0.5f, 0.55f, 0.7f);  // Blueish moonlight
        m_constants.lightIntensity = moonIntensity;
        m_constants.shadowStrength = 0.3f * moonIntensity;  // Softer shadows at night
    }

    // Calculate ambient lighting
    calculateAmbient(timeOfDay);
    m_constants.ambientColorSky = XMFLOAT3(colors.ambientColor.x, colors.ambientColor.y, colors.ambientColor.z);

    // Calculate fog
    calculateFog(timeOfDay);

    // Override with PlanetTheme if provided
    if (theme) {
        AtmosphereSettings atm = theme->getCurrentAtmosphere();
        m_constants.lightColor = XMFLOAT3(atm.sunColor.x, atm.sunColor.y, atm.sunColor.z);
        m_constants.lightIntensity *= atm.sunIntensity;
        m_constants.ambientColorSky = XMFLOAT3(atm.ambientColor.x, atm.ambientColor.y, atm.ambientColor.z);
        m_constants.fogColor = XMFLOAT3(atm.fogColor.x, atm.fogColor.y, atm.fogColor.z);
        m_constants.fogDensity = atm.fogDensity;
    }

    // Apply fog override if set
    if (m_fogOverride) {
        m_constants.fogDensity = m_fogOverrideDensity;
        m_constants.fogColor = m_fogOverrideColor;
    }
}

void GlobalLighting::calculateSunDirection(float timeOfDay)
{
    // Time of day: 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk
    // Sun angle: starts at dawn (0.25), peaks at noon (0.5), sets at dusk (0.75)
    float sunAngle = (timeOfDay - 0.25f) * LOCAL_TWO_PI;

    // Sun elevation (negative Y is down in our coord system)
    float elevation = std::sin(sunAngle) * 0.8f;  // Max 80 degrees from horizon
    float azimuth = std::cos(sunAngle);

    // Light direction is FROM the sun TO the scene (so we negate)
    // Y is up, so positive elevation means sun is above horizon
    m_constants.lightDirection = XMFLOAT3(
        azimuth * 0.8f,   // East-West movement
        -elevation,        // Elevation (negative because light points DOWN from sun)
        0.3f              // Slight tilt for more interesting shadows
    );

    // Normalize
    XMVECTOR dir = XMLoadFloat3(&m_constants.lightDirection);
    dir = XMVector3Normalize(dir);
    XMStoreFloat3(&m_constants.lightDirection, dir);

    // Store sun elevation for shader
    m_constants.sunElevation = elevation;
}

void GlobalLighting::calculateMoonDirection(float timeOfDay, float moonPhase)
{
    // Moon is roughly opposite the sun, with some variation based on phase
    float moonAngle = (timeOfDay - 0.25f + 0.5f) * LOCAL_TWO_PI;  // 12 hours offset from sun
    moonAngle += moonPhase * 0.3f;  // Slight variation based on phase

    float elevation = std::sin(moonAngle) * 0.6f;  // Lower arc than sun
    float azimuth = std::cos(moonAngle);

    m_constants.secondaryLightDir = XMFLOAT3(
        azimuth * 0.9f,
        -elevation,
        -0.2f
    );

    // Normalize
    XMVECTOR dir = XMLoadFloat3(&m_constants.secondaryLightDir);
    dir = XMVector3Normalize(dir);
    XMStoreFloat3(&m_constants.secondaryLightDir, dir);

    // Moon color and intensity
    float moonVisibility = 0.5f * (1.0f - std::cos(moonPhase * LOCAL_TWO_PI));
    m_constants.secondaryLightColor = XMFLOAT3(0.6f, 0.65f, 0.8f);  // Pale blue
    m_constants.secondaryIntensity = moonVisibility * 0.15f;  // Max 15% of sun
}

void GlobalLighting::calculateAmbient(float timeOfDay)
{
    // Sky ambient is stronger during day
    if (timeOfDay > 0.25f && timeOfDay < 0.75f) {
        // Daytime
        m_constants.ambientSkyStrength = 0.3f;
        m_constants.ambientGroundStrength = 0.1f;
        m_constants.ambientColorGround = XMFLOAT3(0.2f, 0.18f, 0.15f);  // Warm brown
    } else {
        // Nighttime - reduced ambient
        m_constants.ambientSkyStrength = 0.08f;
        m_constants.ambientGroundStrength = 0.03f;
        m_constants.ambientColorGround = XMFLOAT3(0.05f, 0.04f, 0.06f);  // Dark cool
    }
}

void GlobalLighting::calculateFog(float timeOfDay)
{
    // Fog is denser at dawn/dusk, lighter at noon and night
    if (timeOfDay > 0.2f && timeOfDay < 0.35f) {
        // Dawn mist
        m_constants.fogDensity = 0.0008f;
        m_constants.fogStart = 30.0f;
        m_constants.fogEnd = 500.0f;
    } else if (timeOfDay > 0.65f && timeOfDay < 0.8f) {
        // Dusk haze
        m_constants.fogDensity = 0.0006f;
        m_constants.fogStart = 40.0f;
        m_constants.fogEnd = 600.0f;
    } else if (timeOfDay > 0.4f && timeOfDay < 0.6f) {
        // Noon - clear
        m_constants.fogDensity = 0.0002f;
        m_constants.fogStart = 100.0f;
        m_constants.fogEnd = 2000.0f;
    } else {
        // Night - moderate
        m_constants.fogDensity = 0.0004f;
        m_constants.fogStart = 50.0f;
        m_constants.fogEnd = 800.0f;
    }
}

// ============================================================================
// Shadow Map Configuration
// ============================================================================

void GlobalLighting::configureShadowMap(ShadowMap_DX12& shadowMap)
{
    // ShadowMap_DX12 uses updateLightSpaceMatrix instead of setLightDirection
    // The actual shadow configuration happens when rendering the shadow pass
    // via updateLightSpaceMatrix(lightDir, sceneCenter, sceneRadius)
    (void)shadowMap;  // Suppress unused parameter warning
}

// ============================================================================
// GPU Operations
// ============================================================================

void GlobalLighting::uploadConstants(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !m_constantBufferMapped) return;
    memcpy(m_constantBufferMapped, &m_constants, sizeof(m_constants));
}

void GlobalLighting::bind(ID3D12GraphicsCommandList* cmdList, uint32_t rootParamIndex)
{
    if (!m_initialized) return;
    cmdList->SetGraphicsRootConstantBufferView(rootParamIndex, m_constantBuffer->GetGPUVirtualAddress());
}

D3D12_GPU_VIRTUAL_ADDRESS GlobalLighting::getConstantBufferGPU() const
{
    return m_constantBuffer ? m_constantBuffer->GetGPUVirtualAddress() : 0;
}

// ============================================================================
// Configuration
// ============================================================================

void GlobalLighting::setWaterLevel(float level)
{
    m_constants.waterLevel = level;
}

void GlobalLighting::setFogOverride(bool enable, float density, const XMFLOAT3& color)
{
    m_fogOverride = enable;
    m_fogOverrideDensity = density;
    m_fogOverrideColor = color;
}

// ============================================================================
// Underwater Lighting
// ============================================================================

XMFLOAT3 GlobalLighting::getUnderwaterAdjustedLight(float depth) const
{
    // Light attenuates with depth, red fastest, blue slowest
    float attenuation = std::exp(-depth * m_constants.underwaterLightFalloff);

    // Wavelength-dependent absorption (red > green > blue)
    XMFLOAT3 color;
    color.x = m_constants.lightColor.x * attenuation * 0.4f;   // Red absorbed quickly
    color.y = m_constants.lightColor.y * attenuation * 0.7f;   // Green moderate
    color.z = m_constants.lightColor.z * attenuation * 0.95f;  // Blue penetrates deepest

    return color;
}
