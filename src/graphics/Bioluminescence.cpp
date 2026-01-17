#include "Bioluminescence.h"
#include "../entities/Creature.h"
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// ============================================================================
// Constructor / Destructor
// ============================================================================

BioluminescenceSystem::BioluminescenceSystem()
{
    memset(&m_constants, 0, sizeof(m_constants));
    m_constants.globalIntensity = 1.0f;
}

BioluminescenceSystem::~BioluminescenceSystem()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool BioluminescenceSystem::init(ID3D12Device* device,
                                  ID3D12DescriptorHeap* srvHeap,
                                  uint32_t srvIndex)
{
    if (!device) {
        std::cerr << "[BioluminescenceSystem] Invalid device!" << std::endl;
        return false;
    }

    m_device = device;

    // Create constant buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = sizeof(BioluminescenceConstants);
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
        std::cerr << "[BioluminescenceSystem] Failed to create constant buffer" << std::endl;
        return false;
    }

    m_constantBuffer->SetName(L"Bioluminescence_ConstantBuffer");

    D3D12_RANGE readRange = {0, 0};
    hr = m_constantBuffer->Map(0, &readRange, &m_constantBufferMapped);
    if (FAILED(hr)) return false;

    // Create structured buffer for glow points
    bufferDesc.Width = sizeof(GlowPointGPU) * MAX_GLOW_POINTS;

    hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_glowPointBuffer)
    );

    if (FAILED(hr)) {
        std::cerr << "[BioluminescenceSystem] Failed to create glow point buffer" << std::endl;
        return false;
    }

    m_glowPointBuffer->SetName(L"Bioluminescence_GlowPointBuffer");

    hr = m_glowPointBuffer->Map(0, &readRange, &m_glowPointBufferMapped);
    if (FAILED(hr)) return false;

    // Create SRV for structured buffer
    if (srvHeap) {
        uint32_t descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_srvCpu = srvHeap->GetCPUDescriptorHandleForHeapStart();
        m_srvCpu.ptr += srvIndex * descriptorSize;
        m_srvGpu = srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_srvGpu.ptr += srvIndex * descriptorSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = MAX_GLOW_POINTS;
        srvDesc.Buffer.StructureByteStride = sizeof(GlowPointGPU);

        m_device->CreateShaderResourceView(m_glowPointBuffer.Get(), &srvDesc, m_srvCpu);
    }

    m_initialized = true;
    std::cout << "[BioluminescenceSystem] Initialized successfully" << std::endl;
    return true;
}

void BioluminescenceSystem::cleanup()
{
    if (m_constantBuffer && m_constantBufferMapped) {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferMapped = nullptr;
    }

    if (m_glowPointBuffer && m_glowPointBufferMapped) {
        m_glowPointBuffer->Unmap(0, nullptr);
        m_glowPointBufferMapped = nullptr;
    }

    m_constantBuffer.Reset();
    m_glowPointBuffer.Reset();
    m_device = nullptr;
    m_initialized = false;
}

// ============================================================================
// Update
// ============================================================================

void BioluminescenceSystem::update(float deltaTime,
                                    const std::vector<Creature*>& creatures,
                                    const DayNightCycle& dayNight)
{
    if (!m_enabled) return;

    m_time += deltaTime;
    m_glowPoints.clear();

    // Calculate darkness factor (0 at noon, 1 at midnight)
    float sunIntensity = dayNight.GetSkyColors().sunIntensity;
    float darkness = 1.0f - std::min(1.0f, sunIntensity * 2.0f);

    // Skip processing during day if nightOnly mode
    if (m_nightOnly && darkness < 0.1f) {
        m_constants.glowPointCount = 0;
        return;
    }

    // Process creatures for bioluminescence
    for (const Creature* creature : creatures) {
        if (creature && creature->hasBioluminescence()) {
            updateCreatureGlow(*creature, darkness);
        }
    }

    // Add manual glow points (vegetation, special effects)
    for (const auto& glow : m_manualGlowPoints) {
        if (m_glowPoints.size() < MAX_GLOW_POINTS) {
            GlowPoint adjusted = glow;
            adjusted.intensity *= darkness * m_globalIntensity;
            m_glowPoints.push_back(adjusted);
        }
    }

    // Sort by intensity (highest first) for importance culling
    std::sort(m_glowPoints.begin(), m_glowPoints.end(),
              [](const GlowPoint& a, const GlowPoint& b) {
                  return a.intensity > b.intensity;
              });

    // Limit to max glow points
    if (m_glowPoints.size() > MAX_GLOW_POINTS) {
        m_glowPoints.resize(MAX_GLOW_POINTS);
    }

    // Update constants
    m_constants.glowPointCount = static_cast<uint32_t>(m_glowPoints.size());
    m_constants.globalIntensity = m_globalIntensity * darkness;
    m_constants.time = m_time;
}

void BioluminescenceSystem::updateCreatureGlow(const Creature& creature, float darkness)
{
    // Get bioluminescence traits from creature's genome
    const Genome& genome = creature.getGenome();

    if (!genome.hasBioluminescence || m_glowPoints.size() >= MAX_GLOW_POINTS) {
        return;
    }

    GlowPoint glow;
    glow.position = XMFLOAT3(
        creature.getPosition().x,
        creature.getPosition().y + creature.getSize() * 0.3f,  // Slightly above center
        creature.getPosition().z
    );

    // Color from genome (with some variation based on creature ID)
    glow.color = XMFLOAT3(
        genome.bioluminescentColor.x,
        genome.bioluminescentColor.y,
        genome.bioluminescentColor.z
    );

    // Intensity based on genome, health, and darkness
    float healthFactor = creature.getEnergy() / creature.getMaxEnergy();
    glow.intensity = genome.glowIntensity * darkness * healthFactor * m_globalIntensity;

    // Radius based on creature size
    glow.radius = creature.getSize() * genome.glowIntensity * 3.0f;

    // Pulse animation
    glow.pulseSpeed = genome.pulseSpeed;
    glow.pulse = calculatePulse(m_time, glow.pulseSpeed, creature.getAge());

    // Apply pulse to intensity
    glow.intensity *= (0.6f + 0.4f * glow.pulse);

    glow.creatureId = creature.getId();

    m_glowPoints.push_back(glow);
}

float BioluminescenceSystem::calculatePulse(float time, float speed, float offset)
{
    // Smooth sinusoidal pulse with slight randomization
    float t = time * speed + offset * 0.1f;
    float primary = std::sin(t) * 0.5f + 0.5f;
    float secondary = std::sin(t * 1.7f + 0.5f) * 0.5f + 0.5f;
    return primary * 0.7f + secondary * 0.3f;
}

// ============================================================================
// GPU Upload
// ============================================================================

void BioluminescenceSystem::uploadToGPU(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized) return;

    // Upload constants
    if (m_constantBufferMapped) {
        memcpy(m_constantBufferMapped, &m_constants, sizeof(m_constants));
    }

    // Upload glow points
    if (m_glowPointBufferMapped && !m_glowPoints.empty()) {
        GlowPointGPU* gpuData = static_cast<GlowPointGPU*>(m_glowPointBufferMapped);

        for (size_t i = 0; i < m_glowPoints.size(); ++i) {
            const GlowPoint& src = m_glowPoints[i];
            gpuData[i].position = src.position;
            gpuData[i].radius = src.radius;
            gpuData[i].color = src.color;
            gpuData[i].intensity = src.intensity;
        }
    }
}

void BioluminescenceSystem::bind(ID3D12GraphicsCommandList* cmdList, uint32_t cbvRootParam, uint32_t srvRootParam)
{
    if (!m_initialized) return;

    cmdList->SetGraphicsRootConstantBufferView(cbvRootParam, m_constantBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(srvRootParam, m_srvGpu);
}

// ============================================================================
// Manual Glow Point Management
// ============================================================================

void BioluminescenceSystem::addGlowPoint(const XMFLOAT3& position,
                                          const XMFLOAT3& color,
                                          float intensity,
                                          float radius)
{
    GlowPoint glow;
    glow.position = position;
    glow.color = color;
    glow.intensity = intensity;
    glow.radius = radius;
    glow.pulse = 1.0f;
    glow.pulseSpeed = 0.0f;
    glow.creatureId = 0;

    m_manualGlowPoints.push_back(glow);
}

void BioluminescenceSystem::clearManualGlowPoints()
{
    m_manualGlowPoints.clear();
}
