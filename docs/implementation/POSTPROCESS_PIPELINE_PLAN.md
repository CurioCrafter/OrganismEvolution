# Post-Processing Pipeline Plan

## Overview
This document describes the post-processing pipeline implementation for the OrganismEvolution project. The pipeline provides visual polish through SSAO, Bloom, Volumetric Fog, SSR, and Tone Mapping.

## Architecture

### Render Target Chain
```
Scene Render → HDR Buffer (R16G16B16A16_FLOAT)
                    ↓
            Depth Copy (R32_FLOAT)
                    ↓
            Normal Generation (R16G16B16A16_FLOAT)
                    ↓
    ┌───────────────┼───────────────┐
    ↓               ↓               ↓
  SSAO           Bloom          Volumetrics
(R8_UNORM)  (R11G11B10_FLOAT)  (R11G11B10_FLOAT)
    ↓               ↓               ↓
    └───────────────┼───────────────┘
                    ↓
            Tone Mapping + Composite
                    ↓
            Back Buffer (R8G8B8A8_UNORM)
```

### Shader Entry Points

| Shader | Entry Point | Description |
|--------|-------------|-------------|
| SSAO.hlsl | CSMain | Main SSAO compute (32 hemisphere samples) |
| SSAO.hlsl | CSBlur | Bilateral blur (9-tap, edge-preserving) |
| Bloom.hlsl | CSExtract | Bright pixel extraction with soft threshold |
| Bloom.hlsl | CSDownsample | 13-tap progressive downsample |
| Bloom.hlsl | CSUpsample | 9-tap tent filter upsample |
| VolumetricFog.hlsl | CSMain | Ray marching (half resolution) |
| VolumetricFog.hlsl | CSUpsample | Bilateral upsample to full res |
| ToneMapping.hlsl | VSMain | Fullscreen triangle vertex shader |
| ToneMapping.hlsl | PSMain | Composition + ACES tone mapping |
| SSR.hlsl | CSMain | HiZ ray marching (optional) |

## Resource Layout

### SRV/UAV Descriptor Indices (from srvStartIndex)
```
 0: HDR Buffer SRV
 1: Depth Copy SRV
 2: Normal Buffer SRV
 3: Normal Buffer UAV
 4: SSAO Buffer SRV
 5: SSAO Buffer UAV
 6: SSAO Blur Buffer SRV
 7: SSAO Blur Buffer UAV
 8: Noise Texture SRV
 9-18: Bloom Mips (5 levels × SRV+UAV)
19: SSR Buffer SRV
20: SSR Buffer UAV
21: Volumetric Buffer SRV
22: Volumetric Buffer UAV
```

### Root Signature Layout (Compute)
```
Slot 0: CBV - Pass constants (b0)
Slot 1: SRV Table - Input textures (t0-t3)
Slot 2: UAV Table - Output texture (u0)
Slot 3: Sampler Table - Linear + Point samplers (s0-s1)
```

## Pipeline Execution Order

### Per-Frame Execution
```cpp
void ExecutePostProcessing(cmdList, depthBuffer, backBuffer) {
    // 1. Copy depth for shader reads
    copyDepthBuffer(cmdList, depthBuffer);

    // 2. Generate normals from depth (if no G-buffer)
    generateNormals(cmdList);

    // 3. SSAO Pass (if enabled)
    if (enableSSAO) {
        renderSSAO(cmdList);
        renderSSAOBlur(cmdList);
    }

    // 4. Bloom Pass (if enabled)
    if (enableBloom) {
        renderBloom(cmdList);
    }

    // 5. Volumetrics Pass (if enabled)
    if (enableVolumetrics) {
        renderVolumetrics(cmdList, ...);
    }

    // 6. SSR Pass (if enabled)
    if (enableSSR) {
        renderSSR(cmdList, ...);
    }

    // 7. Final Composition
    renderToneMapping(cmdList, backBufferRTV);
}
```

## Integration with main.cpp

### Before (Current)
```cpp
void Render() {
    cmdList->SetRenderTargets({backbuffer}, depthBuffer);
    // ... render scene to backbuffer ...
    RenderImGui();
    cmdList->ResourceBarrier(backbuffer, Present);
}
```

### After (With Post-Processing)
```cpp
void Render() {
    // Render scene to HDR buffer
    m_postProcess.beginHDRPass(cmdList, dsvHandle);
    // ... render scene to HDR buffer ...
    m_postProcess.endHDRPass(cmdList);

    // Run post-processing pipeline
    m_postProcess.copyDepthBuffer(cmdList, depthBuffer);
    m_postProcess.generateNormals(cmdList);
    m_postProcess.renderSSAO(cmdList);
    m_postProcess.renderSSAOBlur(cmdList);
    m_postProcess.renderBloom(cmdList);
    m_postProcess.renderVolumetrics(cmdList, invViewProj, cameraPos, lightDir, lightColor);

    // Composite to backbuffer
    m_postProcess.renderToneMapping(cmdList, backbufferRTV);

    // UI renders on top
    RenderImGui();
    cmdList->ResourceBarrier(backbuffer, Present);
}
```

## UI Controls

The following parameters should be exposed in ImGui:

### Post-Processing Panel
- Toggle: SSAO Enabled
- Toggle: Bloom Enabled
- Toggle: Volumetric Fog Enabled
- Toggle: SSR Enabled (advanced)

### SSAO Settings
- Slider: Radius (0.1 - 2.0, default 0.5)
- Slider: Bias (0.0 - 0.1, default 0.025)
- Slider: Intensity (0.5 - 3.0, default 1.0)

### Bloom Settings
- Slider: Threshold (0.5 - 3.0, default 1.0)
- Slider: Intensity (0.0 - 2.0, default 0.5)

### Tone Mapping Settings
- Slider: Exposure (0.1 - 5.0, default 1.0)
- Slider: Gamma (1.8 - 2.4, default 2.2)
- Slider: Saturation (0.0 - 2.0, default 1.0)
- Slider: Contrast (0.5 - 1.5, default 1.0)

### Volumetric Settings
- Slider: Fog Density (0.0 - 0.1, default 0.02)
- Slider: Scattering (0.0 - 0.5, default 0.1)
- Slider: Mie G (0.0 - 0.99, default 0.76)

## Performance Considerations

1. **SSAO**: ~0.5-1.0ms at 1080p with 32 samples
2. **Bloom**: ~0.3-0.5ms (5 mip levels, 2 blur passes each)
3. **Volumetrics**: ~1.0-2.0ms at half resolution
4. **Tone Mapping**: ~0.1ms (single fullscreen quad)
5. **Total**: ~2-4ms overhead expected

## Debug Visualization

Enable individual effect visualization via debug flags:
- `showSSAOOnly`: Display raw SSAO buffer
- `showBloomOnly`: Display bloom contribution
- `showVolumetricsOnly`: Display fog buffer
- Use ToneMapping.hlsl PSDebug entry point

## Implementation Status

- [x] Resource creation (HDR, Depth, SSAO, Bloom, SSR, Volumetric buffers)
- [x] Descriptor allocation (SRV/UAV heap slots)
- [x] SSAO kernel generation
- [x] All HLSL shaders written
- [ ] Root signature creation
- [ ] PSO creation for each pass
- [ ] Compute shader dispatch implementation
- [ ] main.cpp integration
- [ ] ImGui UI controls
- [ ] Performance profiling

## Files Modified/Created

- `src/graphics/PostProcess_DX12.h` - Class definition
- `src/graphics/PostProcess_DX12.cpp` - Implementation
- `src/main.cpp` - Render loop integration
- `shaders/hlsl/SSAO.hlsl` - SSAO compute shader
- `shaders/hlsl/Bloom.hlsl` - Bloom extraction and blur
- `shaders/hlsl/ToneMapping.hlsl` - Final composition
- `shaders/hlsl/VolumetricFog.hlsl` - Volumetric fog
- `shaders/hlsl/SSR.hlsl` - Screen-space reflections
