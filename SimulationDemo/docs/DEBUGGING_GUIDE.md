# DirectX 12 Debugging Guide

Complete guide for debugging DirectX 12 applications with professional tools and workflows.

## Table of Contents

1. [Tool Overview](#tool-overview)
2. [PIX for Windows](#pix-for-windows)
3. [Visual Studio Graphics Diagnostics](#visual-studio-graphics-diagnostics)
4. [RenderDoc](#renderdoc)
5. [D3D12 Debug Layer](#d3d12-debug-layer)
6. [GPU-Based Validation](#gpu-based-validation)
7. [DRED Setup](#dred-device-removed-extended-data)
8. [Debugging Workflows](#debugging-workflows)

---

## Tool Overview

| Tool | DX12 Support | Best For | Cost |
|------|--------------|----------|------|
| **PIX for Windows** | Full | GPU captures, timing, shader debugging | Free |
| **Visual Studio Graphics Diagnostics** | Limited | DX11 debugging, basic frame capture | Free (with VS) |
| **RenderDoc** | Partial | Quick captures, cross-platform | Free |
| **D3D12 Debug Layer** | Full | API validation, error detection | Free |
| **NVIDIA Nsight** | Full | NVIDIA-specific optimizations | Free |
| **AMD Radeon GPU Profiler** | Full | AMD-specific optimizations | Free |

### Recommendation

For DirectX 12 development, **PIX for Windows** is the primary recommended tool. Visual Studio Graphics Diagnostics does not support DX12 fully; Microsoft recommends PIX instead.

---

## PIX for Windows

### Installation

1. **Download PIX**: [PIX Download Page](https://devblogs.microsoft.com/pix/download/)
2. Run the installer (requires Windows 10/11)
3. Install Windows SDK if prompted

### System Requirements

- Windows 10 version 1903 or later
- DirectX 12 compatible GPU
- Latest GPU drivers

### Quick Start

```
1. Launch PIX for Windows
2. Click "Launch Win32" or "Launch UWP"
3. Browse to your executable
4. Click "Launch for GPU Capture"
5. Press PrintScreen (or F11) to capture a frame
6. Double-click the capture to analyze
```

### Configuration for Your Project

Add programmatic capture support to your application:

```cpp
// In your initialization code (reference only - do not modify source files)
#include <pix3.h>

// Load PIX GPU capture DLL for attach support
HMODULE pixModule = LoadLibraryW(L"WinPixGpuCapturer.dll");

// Mark frame boundaries
void Render() {
    PIXBeginEvent(commandQueue, PIX_COLOR(255, 0, 0), "Frame");
    // ... rendering code ...
    PIXEndEvent(commandQueue);
}
```

See [tools/PIX_USAGE.md](tools/PIX_USAGE.md) for detailed PIX workflow.

---

## Visual Studio Graphics Diagnostics

### Important Limitation

> **Visual Studio Graphics Diagnostics does NOT support DirectX 12.**
> For DX12 debugging, use PIX for Windows instead.
> - [Microsoft Documentation](https://learn.microsoft.com/en-us/visualstudio/debugger/graphics/visual-studio-graphics-diagnostics-directx-12)

### When to Use

Visual Studio Graphics Diagnostics is still useful for:
- DirectX 11 applications
- D3D11on12 interop debugging
- Quick DX11 frame captures

### Setup (DX11 Only)

1. Open your project in Visual Studio
2. Go to **Debug > Graphics > Start Graphics Debugging** (Alt+F5)
3. Press **PrintScreen** to capture frames
4. Double-click captures in the Diagnostic Session window

### Features (DX11)

- Frame capture and playback
- Event list inspection
- Pipeline state viewer
- Shader debugging with breakpoints
- Pixel history

---

## RenderDoc

### DX12 Support Status

RenderDoc supports DirectX 12 with some limitations:

| Feature | Support |
|---------|---------|
| Frame capture | Yes |
| API call inspection | Yes |
| Resource viewing | Yes |
| **DXIL/Shader Model 6+** | **No** |
| Texture inspection | Yes |
| Render pass debugging | Yes (since 2019) |

### Critical Limitation

> **RenderDoc does not support DXIL (Shader Model 6.0+)**
>
> If your shaders use SM6+, RenderDoc will report only SM5.1 support.
> Use DXBC fallback shaders or switch to PIX for full DX12 debugging.
> - [RenderDoc D3D12 Documentation](https://renderdoc.org/docs/behind_scenes/d3d12_support.html)

### Installation

1. Download from [RenderDoc.org](https://renderdoc.org/)
2. Run installer
3. No additional configuration needed

### Quick Capture

```
1. Launch RenderDoc
2. File > Launch Application
3. Set executable path and working directory
4. Click "Launch"
5. Press F12 (or PrintScreen) to capture
6. Captures appear in RenderDoc automatically
```

### In-Application Integration

```cpp
// Optional: Programmatic capture control (reference only)
#include "renderdoc_app.h"

RENDERDOC_API_1_1_2* rdoc_api = nullptr;

void InitRenderDoc() {
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
    }
}

void TriggerCapture() {
    if (rdoc_api) rdoc_api->TriggerCapture();
}
```

---

## D3D12 Debug Layer

The debug layer is essential for catching API misuse early.

### Enabling the Debug Layer

#### Windows Requirements

1. **Enable Graphics Tools**:
   - Settings > System > Optional Features > Add a feature
   - Search for "Graphics Tools"
   - Install and restart

2. **Code to Enable** (reference):

```cpp
#ifdef _DEBUG
void EnableDebugLayer() {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();

        // Optional: Enable GPU-based validation
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1))) {
            debugController1->SetEnableGPUBasedValidation(TRUE);
        }
    }
}
#endif
```

### Debug Layer Messages

Common message severity levels:

| Level | Description | Action |
|-------|-------------|--------|
| **ERROR** | Invalid API usage | Must fix immediately |
| **WARNING** | Potential issues | Should investigate |
| **INFO** | Informational | Optional review |
| **MESSAGE** | Debug output | For verbose logging |

### Message Filtering

```cpp
// Filter specific messages (reference only)
void ConfigureDebugLayer(ID3D12Device* device) {
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

        // Suppress specific messages if needed
        D3D12_MESSAGE_ID suppressIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(suppressIds);
        filter.DenyList.pIDList = suppressIds;
        infoQueue->AddStorageFilterEntries(&filter);
    }
}
```

---

## GPU-Based Validation

GPU-Based Validation (GBV) catches errors that CPU-side validation cannot detect.

### What GBV Detects

- Use of uninitialized descriptors in shaders
- Descriptors referencing deleted resources
- Resource state validation
- Out-of-bounds descriptor heap indexing
- Shader access to resources in incompatible states

### Enabling GBV

```cpp
void EnableGPUBasedValidation() {
    ComPtr<ID3D12Debug3> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->SetEnableGPUBasedValidation(TRUE);
        debugController->SetGPUBasedValidationFlags(
            D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
    }
}
```

### Performance Considerations

> **GBV significantly impacts performance**
>
> Best practices:
> - Use with smaller test scenes
> - Enable in nightly test passes
> - Disable for day-to-day development unless tracking specific issues

### GBV Validation Levels

```cpp
// Adjust validation thoroughness
debugController->SetGPUBasedValidationFlags(
    D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING);
```

---

## DRED (Device Removed Extended Data)

DRED helps debug GPU crashes (TDRs / Device Removals).

### What DRED Provides

- **Auto-breadcrumbs**: Track GPU progress before crash
- **Page fault data**: Identify faulting resources
- **PIX event context**: Correlate with debug markers

### Enabling DRED

```cpp
void EnableDRED() {
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)))) {
        // Enable auto-breadcrumbs
        dredSettings->SetAutoBreadcrumbsEnablement(
            D3D12_DRED_ENABLEMENT_FORCED_ON);

        // Enable page fault reporting
        dredSettings->SetPageFaultEnablement(
            D3D12_DRED_ENABLEMENT_FORCED_ON);
    }
}
```

### Reading DRED Data After Crash

```cpp
void HandleDeviceRemoved(ID3D12Device* device) {
    ComPtr<ID3D12DeviceRemovedExtendedData> dred;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dred)))) {
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs;
        D3D12_DRED_PAGE_FAULT_OUTPUT pageFault;

        dred->GetAutoBreadcrumbsOutput(&breadcrumbs);
        dred->GetPageFaultAllocationOutput(&pageFault);

        // Log breadcrumbs to identify last GPU operation
        // Log page fault info to identify bad memory access
    }
}
```

### WinDbg DRED Extension

For post-mortem debugging:
1. Generate crash dump with heap
2. Open in WinDbg
3. Load D3DDBG extension
4. Use `!d3d12dred` commands

---

## Debugging Workflows

### Workflow 1: Black Screen / Nothing Rendering

```
1. Enable Debug Layer
   - Check for any errors/warnings
   - Fix all validation errors first

2. PIX GPU Capture
   - Capture a frame
   - Check if draw calls are present
   - Verify viewport/scissor settings
   - Check render target bindings

3. Verify Pipeline State
   - Check root signature matches shader
   - Verify descriptor tables are populated
   - Check blend state (alpha = 0?)
```

### Workflow 2: Visual Artifacts / Corruption

```
1. Enable GPU-Based Validation
   - Check for resource state mismatches
   - Look for uninitialized descriptor usage

2. PIX Pixel History
   - Select corrupted pixel
   - Trace all draws affecting it
   - Debug suspicious shaders

3. Check Resource States
   - Verify barriers are correct
   - Check for missing transitions
```

### Workflow 3: GPU Crash / TDR

```
1. Enable DRED
   - Set auto-breadcrumbs ON
   - Enable page fault reporting

2. Reproduce Crash
   - Collect DRED data
   - Check last completed operation
   - Identify faulting resource

3. PIX TDR Debugging
   - Capture frame before crash
   - Run GPU validation in PIX
   - Analyze resource lifetimes
```

### Workflow 4: Performance Issues

```
1. PIX Timing Capture
   - Identify slow passes
   - Check GPU utilization
   - Look for pipeline stalls

2. Tracy / Superluminal
   - Profile CPU frame time
   - Check for CPU bottlenecks
   - Analyze thread utilization

3. Vendor Tools
   - NVIDIA Nsight / AMD RGP
   - Check occupancy
   - Analyze shader performance
```

---

## Quick Reference Commands

### Visual Studio

| Shortcut | Action |
|----------|--------|
| Alt+F5 | Start Graphics Debugging (DX11) |
| PrintScreen | Capture Frame |
| F5 | Debug Shader |

### PIX

| Shortcut | Action |
|----------|--------|
| PrintScreen | GPU Capture |
| F11 | GPU Capture (alternate) |
| Ctrl+T | Timing Capture |

### RenderDoc

| Shortcut | Action |
|----------|--------|
| F12 | Capture Frame |
| PrintScreen | Capture Frame (alternate) |
| Ctrl+C | Copy texture |

---

## Additional Resources

### Official Documentation

- [PIX Documentation](https://devblogs.microsoft.com/pix/documentation/)
- [D3D12 Debug Layer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/understanding-the-d3d12-debug-layer)
- [GPU-Based Validation](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation)
- [DRED Specification](https://microsoft.github.io/DirectX-Specs/d3d/DeviceRemovedExtendedData.html)
- [RenderDoc Manual](https://renderdoc.org/docs/)

### Vendor Tools

- [NVIDIA Nsight Graphics](https://developer.nvidia.com/nsight-graphics)
- [AMD Radeon GPU Profiler](https://gpuopen.com/rgp/)
- [Intel GPA](https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/overview.html)

### Community Resources

- [DirectX Discord](https://discord.gg/directx)
- [GameDev.net Graphics Forum](https://www.gamedev.net/forums/forum/technical-graphics-programming/)
