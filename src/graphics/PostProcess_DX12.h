#pragma once

// DirectX 12 Post-Processing Pipeline
// Manages HDR rendering, SSAO, SSR, Bloom, Volumetrics, and Tone Mapping

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
#include <d3dcompiler.h>
#include <cstdint>
#include <array>
#include <memory>
#include <string>

// Forward declarations
// Note: DX12Device forward declaration removed - this class uses ID3D12Device* directly

// ============================================================================
// SSAO Configuration Constants
// ============================================================================
struct SSAOConstants {
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT4X4 invProjection;
    float radius;
    float bias;
    DirectX::XMFLOAT2 noiseScale;
    DirectX::XMFLOAT2 screenSize;
    float intensity;
    float padding;
    DirectX::XMFLOAT4 samples[32];  // Hemisphere kernel samples
};

// ============================================================================
// Bloom Configuration Constants
// ============================================================================
struct BloomConstants {
    float threshold;
    float intensity;
    float filterRadius;
    float padding;
    DirectX::XMFLOAT2 texelSize;
    DirectX::XMFLOAT2 padding2;
};

// ============================================================================
// Tone Mapping Configuration Constants
// ============================================================================
struct ToneMappingConstants {
    float exposure;
    float gamma;
    float saturation;
    float contrast;
    DirectX::XMFLOAT3 lift;
    float padding1;
    DirectX::XMFLOAT3 gain;
    float bloomIntensity;
    DirectX::XMFLOAT2 screenSize;
    uint32_t enableSSAO;
    uint32_t enableBloom;
    uint32_t enableSSR;
    uint32_t enableVolumetrics;
    uint32_t enableFXAA;
    uint32_t enableDistanceFog;
    // FXAA parameters
    float fxaaSubpixelQuality;
    float fxaaEdgeThreshold;
    float fxaaEdgeThresholdMin;
    // Distance fog parameters
    float fogStart;
    float fogEnd;
    float fogDensity;
    DirectX::XMFLOAT3 fogColor;
    float padding2;
};

// ============================================================================
// Time-of-Day Color Grading Constants
// ============================================================================
struct TimeOfDayColorGrading {
    DirectX::XMFLOAT3 shadowTint;         // Tint for dark areas
    float shadowTintStrength;              // 0-1
    DirectX::XMFLOAT3 midtoneTint;        // Tint for midtones
    float midtoneTintStrength;             // 0-1
    DirectX::XMFLOAT3 highlightTint;      // Tint for bright areas
    float highlightTintStrength;           // 0-1
    float colorTemperature;                // -1 cool to +1 warm
    float vignetteIntensity;               // 0-1
    float vignetteRadius;                  // 0-1
    float timeOfDay;                       // For shader reference
};

// ============================================================================
// SSR Configuration Constants
// ============================================================================
struct SSRConstants {
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT4X4 invProjection;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT2 screenSize;
    float maxDistance;
    float thickness;
    int maxSteps;
    int binarySearchSteps;
    float strideZCutoff;
    float padding;
};

// ============================================================================
// Volumetric Fog Configuration Constants
// ============================================================================
struct VolumetricConstants {
    DirectX::XMFLOAT4X4 invViewProj;
    DirectX::XMFLOAT3 cameraPos;
    float fogDensity;
    DirectX::XMFLOAT3 lightDir;
    float scatteringCoeff;
    DirectX::XMFLOAT3 lightColor;
    float absorptionCoeff;
    DirectX::XMFLOAT2 screenSize;
    int numSteps;
    float maxDistance;
    float mieG;  // Mie scattering asymmetry parameter
    DirectX::XMFLOAT3 padding;
};

// ============================================================================
// Underwater Post-Process Constants
// ============================================================================
struct UnderwaterConstants {
    // Fog and visibility
    DirectX::XMFLOAT3 fogColor;           // Underwater fog tint (blue-green)
    float fogDensity;                      // How quickly fog accumulates
    DirectX::XMFLOAT3 absorptionRGB;      // Per-channel light absorption (red absorbs fastest)
    float fogStart;                        // Distance before fog begins
    float fogEnd;                          // Distance at full fog opacity
    float clarityScalar;                   // Multiplier for visibility range
    float underwaterDepth;                 // How deep camera is below surface
    float depthTintStrength;               // How much depth affects tint

    // Light shafts
    DirectX::XMFLOAT2 sunScreenPos;       // Sun position in screen space for shaft origin
    float lightShaftIntensity;             // Strength of god rays
    float lightShaftDecay;                 // Radial falloff

    // Caustics
    float causticIntensity;                // Animated caustic brightness
    float causticScale;                    // Size of caustic pattern
    float time;                            // Animation time
    float surfaceDistortion;               // Distortion when looking at surface from below

    // Screen info
    DirectX::XMFLOAT2 screenSize;
    int qualityLevel;                      // 0=off, 1=low, 2=medium, 3=high
    float padding;
};

// ============================================================================
// Post-Process Manager Class
// ============================================================================
class PostProcessManagerDX12 {
public:
    PostProcessManagerDX12();
    ~PostProcessManagerDX12();

    // Initialize all post-processing resources
    bool init(ID3D12Device* device,
              ID3D12DescriptorHeap* srvUavHeap,
              uint32_t srvStartIndex,
              uint32_t width, uint32_t height);

    void cleanup();

    // Handle window resize
    void resize(uint32_t width, uint32_t height);

    // Create shader PSOs (call after init, requires shader path)
    bool createShaderPipelines(const std::wstring& shaderDirectory);

    // ========================================================================
    // Render Pass Methods
    // ========================================================================

    // Copy depth buffer for post-processing reads
    void copyDepthBuffer(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* sourceDepth);

    // Generate view-space normals from depth (or use G-buffer if available)
    void generateNormals(ID3D12GraphicsCommandList* cmdList);

    // Render SSAO pass (compute shader)
    void renderSSAO(ID3D12GraphicsCommandList* cmdList);

    // Render SSAO bilateral blur
    void renderSSAOBlur(ID3D12GraphicsCommandList* cmdList);

    // Render SSR pass (compute shader)
    void renderSSR(ID3D12GraphicsCommandList* cmdList,
                   ID3D12Resource* colorBuffer,
                   const DirectX::XMMATRIX& view,
                   const DirectX::XMMATRIX& projection);

    // Render volumetric fog/god rays
    void renderVolumetrics(ID3D12GraphicsCommandList* cmdList,
                           const DirectX::XMMATRIX& invViewProj,
                           const DirectX::XMFLOAT3& cameraPos,
                           const DirectX::XMFLOAT3& lightDir,
                           const DirectX::XMFLOAT3& lightColor);

    // Render underwater post-process effects (fog, absorption, caustics, light shafts)
    void renderUnderwater(ID3D12GraphicsCommandList* cmdList,
                          float underwaterDepth,
                          const DirectX::XMFLOAT2& sunScreenPos,
                          float time);

    // Render bloom extraction + blur
    void renderBloom(ID3D12GraphicsCommandList* cmdList);

    // Final tone mapping and composition
    void renderToneMapping(ID3D12GraphicsCommandList* cmdList,
                           D3D12_CPU_DESCRIPTOR_HANDLE outputRTV);

    // ========================================================================
    // HDR Buffer Access
    // ========================================================================

    // Get HDR render target for main pass rendering
    D3D12_CPU_DESCRIPTOR_HANDLE getHDRRTVHandle() const { return m_hdrRTVHandle; }
    ID3D12Resource* getHDRBuffer() const { return m_hdrBuffer.Get(); }

    // Begin/end HDR rendering (sets up render target)
    void beginHDRPass(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE depthDSV);
    void endHDRPass(ID3D12GraphicsCommandList* cmdList);

    // ========================================================================
    // Effect Toggle Properties
    // ========================================================================

    bool enableSSAO = true;
    bool enableBloom = true;
    bool enableSSR = false;       // Disabled by default (expensive)
    bool enableVolumetrics = false; // Disabled by default (expensive)
    bool enableFXAA = true;       // FXAA anti-aliasing (fast)
    bool enableDistanceFog = true; // Distance fog for LOD transitions

    // ========================================================================
    // SSAO Parameters
    // ========================================================================

    float ssaoRadius = 0.5f;
    float ssaoBias = 0.025f;
    float ssaoIntensity = 1.0f;
    int ssaoKernelSize = 32;

    // ========================================================================
    // Bloom Parameters
    // ========================================================================

    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;
    int bloomMipLevels = 5;

    // ========================================================================
    // Tone Mapping Parameters
    // ========================================================================

    float exposure = 1.0f;
    float gamma = 2.2f;
    float saturation = 1.0f;
    float contrast = 1.0f;
    DirectX::XMFLOAT3 colorLift = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 colorGain = {1.0f, 1.0f, 1.0f};

    // ========================================================================
    // SSR Parameters
    // ========================================================================

    float ssrMaxDistance = 50.0f;
    float ssrThickness = 0.1f;
    int ssrMaxSteps = 64;
    int ssrBinarySearchSteps = 8;

    // ========================================================================
    // Volumetric Parameters
    // ========================================================================

    float fogDensity = 0.02f;
    float fogScattering = 0.1f;
    float fogAbsorption = 0.01f;
    float mieG = 0.76f;  // Forward scattering (0 = isotropic, 1 = forward)
    int volumetricSteps = 32;
    float volumetricMaxDistance = 200.0f;

    // ========================================================================
    // FXAA Parameters
    // ========================================================================

    float fxaaSubpixelQuality = 0.75f;     // 0.0 = off, 1.0 = softer
    float fxaaEdgeThreshold = 0.166f;       // Lower = more edges detected
    float fxaaEdgeThresholdMin = 0.0833f;   // Skip low contrast edges

    // ========================================================================
    // Distance Fog Parameters (for LOD transitions)
    // ========================================================================

    float distanceFogStart = 400.0f;
    float distanceFogEnd = 2000.0f;
    float distanceFogDensity = 0.0008f;
    DirectX::XMFLOAT3 distanceFogColor = {0.7f, 0.8f, 0.9f};

    // ========================================================================
    // Underwater Parameters
    // ========================================================================

    bool enableUnderwater = true;                                  // Master toggle for underwater effects
    DirectX::XMFLOAT3 underwaterFogColor = {0.0f, 0.15f, 0.3f};   // Deep blue fog
    float underwaterFogDensity = 0.02f;                            // Fog accumulation rate
    DirectX::XMFLOAT3 underwaterAbsorption = {0.4f, 0.15f, 0.05f}; // RGB absorption (red fastest)
    float underwaterFogStart = 5.0f;                               // Distance before fog starts
    float underwaterFogEnd = 150.0f;                               // Distance at full fog
    float underwaterClarity = 1.0f;                                // Visibility multiplier
    float underwaterDepthTint = 0.3f;                              // Depth-based tint strength
    float underwaterCausticIntensity = 0.3f;                       // Caustic animation strength
    float underwaterCausticScale = 0.02f;                          // Caustic pattern size
    float underwaterLightShaftIntensity = 0.4f;                    // Sun shaft strength
    float underwaterLightShaftDecay = 0.95f;                       // Shaft radial falloff
    float underwaterSurfaceDistortion = 0.02f;                     // Distortion looking up at surface
    int underwaterQuality = 1;                                     // 0=off, 1=low, 2=med, 3=high

    // ========================================================================
    // Time-of-Day Color Grading
    // ========================================================================

    TimeOfDayColorGrading colorGrading;
    bool enableTimeOfDayGrading = true;

    // Set color grading from DayNightCycle
    void updateColorGrading(float timeOfDay);
    void setColorGradingPreset(float dawn, float noon, float dusk, float night);

    // ========================================================================
    // Debug Options
    // ========================================================================

    bool showSSAOOnly = false;
    bool showBloomOnly = false;
    bool showSSROnly = false;
    bool showVolumetricsOnly = false;

    // ========================================================================
    // Resource Accessors
    // ========================================================================

    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    bool isInitialized() const { return m_initialized; }

    // SRV handles for shader binding
    D3D12_GPU_DESCRIPTOR_HANDLE getSSAOSRVHandle() const { return m_ssaoBlurSRVGpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE getBloomSRVHandle() const { return m_bloomSRVGpu[0]; }
    D3D12_GPU_DESCRIPTOR_HANDLE getSSRSRVHandle() const { return m_ssrSRVGpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE getVolumetricsSRVHandle() const { return m_volumetricSRVGpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE getUnderwaterSRVHandle() const { return m_underwaterSRVGpu; }

private:
    // ========================================================================
    // Resource Creation Helpers
    // ========================================================================

    bool createHDRBuffer();
    bool createDepthCopy();
    bool createNormalBuffer();
    bool createSSAOBuffers();
    bool createBloomBuffers();
    bool createSSRBuffer();
    bool createVolumetricBuffer();
    bool createUnderwaterBuffer();
    bool createNoiseTexture();
    bool createSSAOKernel();

    void releaseResources();

    // ========================================================================
    // PSO and Root Signature Creation
    // ========================================================================

    bool createRootSignatures();
    bool createComputePSOs();
    bool createGraphicsPSOs();
    bool compileShader(const std::wstring& path, const std::string& entryPoint,
                       const std::wstring& target, ID3DBlob** blob);

    // Constant buffer for per-pass data
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferMapped = nullptr;
    uint32_t m_constantBufferSize = 0;

    // ========================================================================
    // D3D12 Resources
    // ========================================================================

    // Device reference
    ID3D12Device* m_device = nullptr;
    ID3D12DescriptorHeap* m_srvUavHeap = nullptr;
    uint32_t m_srvStartIndex = 0;
    uint32_t m_srvDescriptorSize = 0;

    // Dimensions
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // HDR Buffer (main scene rendering target)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_hdrBuffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_hdrRTVHeap;  // Private RTV heap for HDR
    D3D12_CPU_DESCRIPTOR_HANDLE m_hdrRTVHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_hdrSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_hdrSRVGpu = {};
    D3D12_RESOURCE_STATES m_hdrState = D3D12_RESOURCE_STATE_COMMON;

    // Depth copy (for SSAO/SSR reads)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthCopy;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthCopySRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_depthCopySRVGpu = {};

    // Normal buffer (view-space normals)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_normalSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_normalSRVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_normalUAVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_normalUAVGpu = {};

    // SSAO buffers
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ssaoBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ssaoBlurBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_ssaoSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssaoSRVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_ssaoUAVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssaoUAVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_ssaoBlurSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssaoBlurSRVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_ssaoBlurUAVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssaoBlurUAVGpu = {};

    // SSAO noise texture
    Microsoft::WRL::ComPtr<ID3D12Resource> m_noiseTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_noiseSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_noiseSRVGpu = {};

    // SSAO kernel (stored in constant buffer)
    std::array<DirectX::XMFLOAT4, 32> m_ssaoKernel;

    // Bloom buffers (mip chain)
    static constexpr int MAX_BLOOM_MIPS = 5;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_bloomBuffers[MAX_BLOOM_MIPS];
    D3D12_CPU_DESCRIPTOR_HANDLE m_bloomSRVCpu[MAX_BLOOM_MIPS] = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_bloomSRVGpu[MAX_BLOOM_MIPS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_bloomUAVCpu[MAX_BLOOM_MIPS] = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_bloomUAVGpu[MAX_BLOOM_MIPS] = {};

    // SSR buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ssrBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_ssrSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssrSRVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_ssrUAVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ssrUAVGpu = {};

    // Volumetric buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> m_volumetricBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_volumetricSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_volumetricSRVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_volumetricUAVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_volumetricUAVGpu = {};

    // Underwater buffer (for caustics and light shafts)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_underwaterBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_underwaterSRVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_underwaterSRVGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_underwaterUAVCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_underwaterUAVGpu = {};

    // Pipeline State Objects (to be loaded from shaders)
    // These would be created during shader compilation
    // For now, kept as placeholders
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ssaoPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ssaoBlurPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bloomExtractPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bloomBlurPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ssrPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_volumetricPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_underwaterPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_toneMappingPSO;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_computeRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_graphicsRootSignature;

    bool m_initialized = false;
};
