#include "SkyRenderer.h"
#include "Camera.h"
#include "../environment/PlanetTheme.h"
#include <d3dcompiler.h>
#include <iostream>
#include <cmath>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// ============================================================================
// Constructor / Destructor
// ============================================================================

SkyRenderer::SkyRenderer()
{
    // Initialize constants to sensible defaults
    memset(&m_constants, 0, sizeof(m_constants));
    m_constants.sunSize = m_sunSize;
    m_constants.moonSize = m_moonSize;
    m_constants.rayleighStrength = m_rayleighStrength;
    m_constants.mieStrength = m_mieStrength;
    m_constants.mieG = m_mieG;
    m_constants.atmosphereHeight = 100000.0f;
}

SkyRenderer::~SkyRenderer()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool SkyRenderer::init(ID3D12Device* device,
                        ID3D12DescriptorHeap* srvHeap,
                        uint32_t srvIndex,
                        DXGI_FORMAT renderTargetFormat)
{
    if (!device) {
        std::cerr << "[SkyRenderer] Invalid device!" << std::endl;
        return false;
    }

    m_device = device;

    if (!createConstantBuffer()) {
        std::cerr << "[SkyRenderer] Failed to create constant buffer!" << std::endl;
        return false;
    }

    if (!createPipelineState(renderTargetFormat)) {
        std::cerr << "[SkyRenderer] Failed to create pipeline state!" << std::endl;
        // Continue anyway - PSO may be created later by shader manager
    }

    m_initialized = true;
    std::cout << "[SkyRenderer] Initialized successfully" << std::endl;
    return true;
}

void SkyRenderer::cleanup()
{
    if (m_constantBuffer && m_constantBufferMapped) {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferMapped = nullptr;
    }

    m_constantBuffer.Reset();
    m_pipelineState.Reset();
    m_rootSignature.Reset();
    m_device = nullptr;
    m_initialized = false;
}

// ============================================================================
// Pipeline State Creation
// ============================================================================

bool SkyRenderer::createPipelineState(DXGI_FORMAT renderTargetFormat)
{
    // Create root signature for sky rendering
    // [0] CBV - Sky constants (b0)
    D3D12_ROOT_PARAMETER1 rootParams[1] = {};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Static samplers (s0 - linear)
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].RegisterSpace = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rsDesc.Desc_1_1.NumParameters = 1;
    rsDesc.Desc_1_1.pParameters = rootParams;
    rsDesc.Desc_1_1.NumStaticSamplers = 1;
    rsDesc.Desc_1_1.pStaticSamplers = staticSamplers;
    rsDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            std::cerr << "[SkyRenderer] RS error: " << (char*)error->GetBufferPointer() << std::endl;
        }
        return false;
    }

    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                        IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr)) {
        std::cerr << "[SkyRenderer] Failed to create root signature" << std::endl;
        return false;
    }

    // Note: PSO creation deferred to shader manager which handles HLSL compilation
    // The shader Sky.hlsl will use VSMain and PSMain entry points
    std::cout << "[SkyRenderer] Root signature created, PSO deferred to shader manager" << std::endl;

    return true;
}

bool SkyRenderer::createConstantBuffer()
{
    // Create upload heap for constant buffer (256-byte aligned)
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = sizeof(SkyConstants);
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
        std::cerr << "[SkyRenderer] Failed to create constant buffer" << std::endl;
        return false;
    }

    m_constantBuffer->SetName(L"Sky_ConstantBuffer");

    // Map for CPU writes
    D3D12_RANGE readRange = {0, 0};
    hr = m_constantBuffer->Map(0, &readRange, &m_constantBufferMapped);
    if (FAILED(hr)) {
        std::cerr << "[SkyRenderer] Failed to map constant buffer" << std::endl;
        return false;
    }

    return true;
}

// ============================================================================
// Update
// ============================================================================

void SkyRenderer::update(const DayNightCycle& dayNight, const PlanetTheme* theme)
{
    m_time += 0.016f;  // Approximate time for shader animation

    // Get sky colors from day/night cycle
    SkyColors colors = dayNight.GetSkyColors();

    // Sun direction from day/night cycle
    Vec3 sunPos = dayNight.GetSunPosition();
    Vec3 sunDir = sunPos.Normalized();
    m_constants.sunDirection = XMFLOAT3(sunDir.x, sunDir.y, sunDir.z);
    m_constants.sunIntensity = colors.sunIntensity;
    m_constants.sunColor = XMFLOAT3(colors.sunColor.x, colors.sunColor.y, colors.sunColor.z);
    m_constants.sunSize = m_sunSize;

    // Moon direction (opposite to sun for simplicity, with phase offset)
    Vec3 moonPos = dayNight.GetMoonPosition();
    Vec3 moonDir = moonPos.Normalized();
    m_constants.moonDirection = XMFLOAT3(moonDir.x, moonDir.y, moonDir.z);
    m_constants.moonPhase = dayNight.GetMoonPhase();
    m_constants.moonColor = XMFLOAT3(0.8f, 0.85f, 0.95f);  // Pale blue-white moonlight
    m_constants.moonSize = m_moonSize;

    // Sky colors
    m_constants.zenithColor = XMFLOAT3(colors.skyTop.x, colors.skyTop.y, colors.skyTop.z);
    m_constants.horizonColor = XMFLOAT3(colors.skyHorizon.x, colors.skyHorizon.y, colors.skyHorizon.z);
    m_constants.starVisibility = dayNight.GetStarVisibility();

    // Fog color matches horizon at night, uses ambient during day
    if (dayNight.IsNight()) {
        m_constants.fogColor = m_constants.horizonColor;
    } else {
        m_constants.fogColor = XMFLOAT3(colors.ambientColor.x * 1.5f,
                                         colors.ambientColor.y * 1.5f,
                                         colors.ambientColor.z * 1.5f);
    }
    m_constants.fogDensity = dayNight.IsNight() ? 0.001f : 0.0005f;

    // Atmosphere params
    m_constants.rayleighStrength = m_rayleighStrength;
    m_constants.mieStrength = m_mieStrength;
    m_constants.mieG = m_mieG;
    m_constants.atmosphereHeight = 100000.0f;
    m_constants.time = m_time;

    // Override with PlanetTheme if provided
    if (theme) {
        AtmosphereSettings atm = theme->getCurrentAtmosphere();
        m_constants.zenithColor = XMFLOAT3(atm.skyZenithColor.x, atm.skyZenithColor.y, atm.skyZenithColor.z);
        m_constants.horizonColor = XMFLOAT3(atm.skyHorizonColor.x, atm.skyHorizonColor.y, atm.skyHorizonColor.z);
        m_constants.sunColor = XMFLOAT3(atm.sunColor.x, atm.sunColor.y, atm.sunColor.z);
        m_constants.fogColor = XMFLOAT3(atm.fogColor.x, atm.fogColor.y, atm.fogColor.z);
        m_constants.fogDensity = atm.fogDensity;
    }

    // ========================================================================
    // Apply Visual Style Color Grading (No Man's Sky inspired)
    // ========================================================================
    if (m_visualStyle.styleStrength > 0.0f) {
        // Apply color grading to sky colors
        applyColorGrading(m_constants.zenithColor);
        applyColorGrading(m_constants.horizonColor);

        // Enhance sun color with warmth
        m_constants.sunColor.x *= 1.0f + m_visualStyle.sunGlowIntensity * 0.1f;
        m_constants.sunColor.y *= 1.0f + m_visualStyle.sunGlowIntensity * 0.05f;

        // Apply horizon glow - add subtle warmth to horizon
        float horizonGlow = m_visualStyle.horizonGlow * m_visualStyle.styleStrength;
        m_constants.horizonColor.x += horizonGlow * 0.08f;
        m_constants.horizonColor.y += horizonGlow * 0.04f;
        m_constants.horizonColor.z -= horizonGlow * 0.02f;

        // Clamp colors
        m_constants.horizonColor.x = std::min(m_constants.horizonColor.x, 1.0f);
        m_constants.horizonColor.y = std::min(m_constants.horizonColor.y, 1.0f);
        m_constants.horizonColor.z = std::max(m_constants.horizonColor.z, 0.0f);

        // Enhance contrast in sky gradient
        float contrastFactor = m_visualStyle.skyGradientContrast;
        // Make zenith darker and horizon brighter for more dramatic sky
        float zenithLum = (m_constants.zenithColor.x + m_constants.zenithColor.y + m_constants.zenithColor.z) / 3.0f;
        float darkening = (1.0f - (contrastFactor - 1.0f) * 0.3f);
        m_constants.zenithColor.x *= darkening;
        m_constants.zenithColor.y *= darkening;
        m_constants.zenithColor.z *= darkening;
    }
}

void SkyRenderer::updateConstants(const Camera& camera)
{
    // Update camera-related constants
    // Note: Camera class uses lowercase method names and getProjectionMatrix needs aspect ratio
    glm::mat4 viewMat = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 projMat = camera.getProjectionMatrix(16.0f / 9.0f);  // Default aspect ratio
    XMMATRIX view = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&viewMat));
    XMMATRIX proj = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&projMat));
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);
    XMStoreFloat4x4(&m_constants.invViewProj, XMMatrixTranspose(invViewProj));

    m_constants.cameraPosition = XMFLOAT3(camera.Position.x, camera.Position.y, camera.Position.z);

    // Upload to GPU
    if (m_constantBufferMapped) {
        memcpy(m_constantBufferMapped, &m_constants, sizeof(m_constants));
    }
}

// ============================================================================
// Rendering
// ============================================================================

void SkyRenderer::render(ID3D12GraphicsCommandList* cmdList,
                          const Camera& camera,
                          D3D12_CPU_DESCRIPTOR_HANDLE renderTarget,
                          D3D12_CPU_DESCRIPTOR_HANDLE depthStencil)
{
    if (!m_initialized || !cmdList) return;

    // Update camera-dependent constants
    updateConstants(camera);

    // Set render target
    cmdList->OMSetRenderTargets(1, &renderTarget, FALSE, &depthStencil);

    // If PSO is ready, render the sky dome
    if (m_pipelineState && m_rootSignature) {
        cmdList->SetPipelineState(m_pipelineState.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

        // Bind constant buffer
        cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

        // Draw fullscreen triangle (sky shader generates vertices from SV_VertexID)
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}

void SkyRenderer::renderForReflection(ID3D12GraphicsCommandList* cmdList,
                                       const XMMATRIX& view,
                                       const XMMATRIX& projection)
{
    if (!m_initialized || !cmdList) return;

    // Update inv view-proj for reflection camera
    XMMATRIX viewProj = XMMatrixMultiply(view, projection);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);
    XMStoreFloat4x4(&m_constants.invViewProj, XMMatrixTranspose(invViewProj));

    // Upload to GPU
    if (m_constantBufferMapped) {
        memcpy(m_constantBufferMapped, &m_constants, sizeof(m_constants));
    }

    // Render sky
    if (m_pipelineState && m_rootSignature) {
        cmdList->SetPipelineState(m_pipelineState.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}

// ============================================================================
// Configuration
// ============================================================================

void SkyRenderer::setAtmosphereParams(float rayleigh, float mie, float mieG)
{
    m_rayleighStrength = rayleigh;
    m_mieStrength = mie;
    m_mieG = mieG;
}

// ============================================================================
// Visual Style - Color Grading
// ============================================================================

void SkyRenderer::applyColorGrading(XMFLOAT3& color) const
{
    // Apply color filter
    color.x *= m_visualStyle.colorFilter.x;
    color.y *= m_visualStyle.colorFilter.y;
    color.z *= m_visualStyle.colorFilter.z;

    // Calculate luminance
    float luminance = color.x * 0.299f + color.y * 0.587f + color.z * 0.114f;

    // Apply shadow warmth (add warmth to darker areas)
    float shadowFactor = 1.0f - luminance;
    shadowFactor = shadowFactor * shadowFactor;  // Quadratic falloff
    color.x += shadowFactor * m_visualStyle.shadowWarmth * 0.1f;
    color.y += shadowFactor * m_visualStyle.shadowWarmth * 0.05f;
    // Blue stays the same for warm shadows

    // Apply highlight cooling (add cool tint to bright areas)
    float highlightFactor = luminance;
    highlightFactor = highlightFactor * highlightFactor;  // Quadratic
    color.z += highlightFactor * m_visualStyle.highlightCool * 0.1f;
    color.y += highlightFactor * m_visualStyle.highlightCool * 0.02f;
    // Red stays the same for cool highlights

    // Apply color vibrancy (saturation boost)
    float desaturated = luminance;
    color.x = desaturated + (color.x - desaturated) * m_visualStyle.colorVibrancy;
    color.y = desaturated + (color.y - desaturated) * m_visualStyle.colorVibrancy;
    color.z = desaturated + (color.z - desaturated) * m_visualStyle.colorVibrancy;

    // Add theme tint
    color.x += m_visualStyle.themeTint.x * m_visualStyle.styleStrength * 0.1f;
    color.y += m_visualStyle.themeTint.y * m_visualStyle.styleStrength * 0.1f;
    color.z += m_visualStyle.themeTint.z * m_visualStyle.styleStrength * 0.1f;

    // Clamp to valid range
    color.x = std::min(std::max(color.x, 0.0f), 1.0f);
    color.y = std::min(std::max(color.y, 0.0f), 1.0f);
    color.z = std::min(std::max(color.z, 0.0f), 1.0f);
}
