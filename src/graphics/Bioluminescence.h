#pragma once

// Bioluminescence.h - Dynamic creature glow system for night-time rendering
// Manages point lights from bioluminescent creatures and vegetation

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
#include <vector>
#include <cstdint>
#include "../core/DayNightCycle.h"

// Forward declarations
class Creature;

// ============================================================================
// Glow Point - Single bioluminescent light source
// ============================================================================
struct GlowPoint
{
    DirectX::XMFLOAT3 position;   // World position
    float radius;                  // Light falloff radius
    DirectX::XMFLOAT3 color;      // Light color (HDR)
    float intensity;               // Light intensity (0-10+)
    float pulse;                   // Current pulse value (0-1)
    float pulseSpeed;              // Pulse animation speed
    uint32_t creatureId;          // Source creature ID (for tracking)
};

// ============================================================================
// Bioluminescence Constants (for GPU upload)
// ============================================================================
static constexpr uint32_t MAX_GLOW_POINTS = 256;

struct alignas(16) GlowPointGPU
{
    DirectX::XMFLOAT3 position;
    float radius;
    DirectX::XMFLOAT3 color;
    float intensity;
};

struct alignas(256) BioluminescenceConstants
{
    uint32_t glowPointCount;
    float globalIntensity;        // Day/night multiplier
    float time;                   // For pulse animation
    float padding;

    // Glow points are in a separate structured buffer
};

// ============================================================================
// Bioluminescence System
// ============================================================================
class BioluminescenceSystem
{
public:
    BioluminescenceSystem();
    ~BioluminescenceSystem();

    // Initialize DX12 resources
    bool init(ID3D12Device* device,
              ID3D12DescriptorHeap* srvHeap,
              uint32_t srvIndex);
    void cleanup();

    // Update glow points from creatures
    void update(float deltaTime,
                const std::vector<Creature*>& creatures,
                const DayNightCycle& dayNight);

    // Upload to GPU (call before rendering)
    void uploadToGPU(ID3D12GraphicsCommandList* cmdList);

    // Bind to shader
    void bind(ID3D12GraphicsCommandList* cmdList, uint32_t cbvRootParam, uint32_t srvRootParam);

    // Query
    uint32_t getActiveGlowCount() const { return static_cast<uint32_t>(m_glowPoints.size()); }
    const std::vector<GlowPoint>& getGlowPoints() const { return m_glowPoints; }

    // Manual glow point management (for vegetation, special effects)
    void addGlowPoint(const DirectX::XMFLOAT3& position,
                      const DirectX::XMFLOAT3& color,
                      float intensity,
                      float radius);
    void clearManualGlowPoints();

    // Configuration
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void setNightOnlyMode(bool nightOnly) { m_nightOnly = nightOnly; }
    void setGlobalIntensity(float intensity) { m_globalIntensity = intensity; }

    bool isInitialized() const { return m_initialized; }

private:
    void updateCreatureGlow(const Creature& creature, float darkness);
    float calculatePulse(float time, float speed, float offset);

    // DX12 resources
    ID3D12Device* m_device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_glowPointBuffer;  // Structured buffer
    void* m_constantBufferMapped = nullptr;
    void* m_glowPointBufferMapped = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu;

    // Current state
    std::vector<GlowPoint> m_glowPoints;
    std::vector<GlowPoint> m_manualGlowPoints;
    BioluminescenceConstants m_constants;

    // Configuration
    bool m_enabled = true;
    bool m_nightOnly = true;
    float m_globalIntensity = 1.0f;
    float m_time = 0.0f;

    bool m_initialized = false;
};
