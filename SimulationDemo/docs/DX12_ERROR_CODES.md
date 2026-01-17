# DirectX 12 Error Codes Reference

A comprehensive reference for all common DX12 and DXGI error codes with plain English explanations, common causes, and fix strategies.

## Table of Contents

1. [How to Read HRESULT Values](#how-to-read-hresult-values)
2. [Success Codes](#success-codes)
3. [DXGI Errors](#dxgi-errors)
4. [D3D12 Specific Errors](#d3d12-specific-errors)
5. [Common Win32 Errors](#common-win32-errors)
6. [Error Lookup Utility](#error-lookup-utility)
7. [Troubleshooting Checklists](#troubleshooting-checklists)

---

## How to Read HRESULT Values

HRESULT is a 32-bit value with this structure:

```
Bit 31 (S): Severity (0 = Success, 1 = Error)
Bits 30-16: Facility code (identifies the source)
Bits 15-0: Error code within the facility
```

### DXGI Error Format

DXGI errors use facility code `0x87a`:
```cpp
#define _FACDXGI 0x87a
// DXGI_ERROR_INVALID_CALL = 0x887A0001
//                           ^^|^^^|^^^^
//                           S |Fac|Code
//                           8=Error, 87A=DXGI, 0001=Code
```

### Quick Test Macros

```cpp
#include <winerror.h>

SUCCEEDED(hr)  // Returns TRUE if hr indicates success
FAILED(hr)     // Returns TRUE if hr indicates failure
```

---

## Success Codes

| Code | Hex Value | Description |
|------|-----------|-------------|
| **S_OK** | `0x00000000` | Operation completed successfully |
| **S_FALSE** | `0x00000001` | Success, but with a non-standard outcome (e.g., no items found, already done) |

---

## DXGI Errors

### DXGI_ERROR_INVALID_CALL

| | |
|---|---|
| **Hex Value** | `0x887A0001` |
| **Plain English** | You called a DirectX function with invalid parameters |
| **Common Causes** | - Null pointer passed where valid pointer required<br>- Invalid combination of flags<br>- Resource in wrong state for operation<br>- Invalid format specified |
| **Fix Checklist** | ☐ Enable D3D12 Debug Layer to get detailed error messages<br>☐ Check all pointer parameters are valid<br>☐ Verify resource states with PIX or RenderDoc<br>☐ Review API documentation for valid parameter combinations |
| **Prevention** | Always validate parameters before D3D calls; use Debug Layer during development |

---

### DXGI_ERROR_NOT_FOUND

| | |
|---|---|
| **Hex Value** | `0x887A0002` |
| **Plain English** | The requested item doesn't exist |
| **Common Causes** | - GetPrivateData called with unknown GUID<br>- EnumAdapters/EnumOutputs with out-of-range index<br>- Requested feature not available |
| **Fix Checklist** | ☐ Verify GUID is correct for GetPrivateData<br>☐ Check adapter/output count before enumeration<br>☐ Use CheckFeatureSupport before using advanced features |
| **Prevention** | Always enumerate available items before accessing by index |

---

### DXGI_ERROR_MORE_DATA

| | |
|---|---|
| **Hex Value** | `0x887A0003` |
| **Plain English** | Your buffer is too small to hold the returned data |
| **Common Causes** | - Buffer size underestimated<br>- Not querying required size first |
| **Fix Checklist** | ☐ Call function first with null buffer to get required size<br>☐ Allocate buffer of returned size<br>☐ Call again with properly sized buffer |
| **Prevention** | Always query required buffer size before allocating |

```cpp
// Correct pattern:
UINT size = 0;
object->GetPrivateData(guid, &size, nullptr);  // Get size
std::vector<BYTE> data(size);
object->GetPrivateData(guid, &size, data.data());  // Get data
```

---

### DXGI_ERROR_UNSUPPORTED

| | |
|---|---|
| **Hex Value** | `0x887A0004` |
| **Plain English** | This feature or operation is not supported by the hardware/driver |
| **Common Causes** | - Requesting feature not supported by GPU<br>- Driver too old<br>- Wrong shader model for hardware<br>- Unsupported texture format |
| **Fix Checklist** | ☐ Use CheckFeatureSupport to verify capability<br>☐ Update GPU drivers<br>☐ Check minimum hardware requirements<br>☐ Implement fallback path for older hardware |
| **Prevention** | Always check feature support before using; design with fallbacks |

```cpp
// Check before using:
D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
if (SUCCEEDED(device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
    if (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0) {
        // Safe to use raytracing
    }
}
```

---

### DXGI_ERROR_DEVICE_REMOVED

| | |
|---|---|
| **Hex Value** | `0x887A0005` |
| **Plain English** | The GPU has been disconnected, crashed, or the driver was updated |
| **Common Causes** | - GPU driver crash (TDR)<br>- Driver update during runtime<br>- GPU physically removed (laptops with eGPU)<br>- GPU overheating and throttling<br>- Hardware failure |
| **Fix Checklist** | ☐ Call `GetDeviceRemovedReason()` for more info<br>☐ Check DRED data if enabled<br>☐ Update GPU drivers<br>☐ Check for GPU overheating<br>☐ Implement device recreation logic |
| **Prevention** | - Check Present() return value every frame<br>- Enable DRED for debugging<br>- Implement proper device recreation |

```cpp
// Get detailed reason:
HRESULT reason = device->GetDeviceRemovedReason();
switch (reason) {
    case DXGI_ERROR_DEVICE_HUNG:
        // GPU hung processing commands
        break;
    case DXGI_ERROR_DEVICE_RESET:
        // Runtime error, device needs recreation
        break;
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        // Driver bug
        break;
    // ... handle other cases
}
```

---

### DXGI_ERROR_DEVICE_HUNG

| | |
|---|---|
| **Hex Value** | `0x887A0006` |
| **Plain English** | The GPU stopped responding while processing commands |
| **Common Causes** | - Infinite loop in shader<br>- Too complex shader (exceeds TDR timeout)<br>- Invalid memory access in shader<br>- Resource barrier issues causing GPU stall |
| **Fix Checklist** | ☐ Enable DRED to find problematic command<br>☐ Check shaders for infinite loops<br>☐ Reduce shader complexity<br>☐ Verify resource barriers are correct<br>☐ Check for UAV hazards |
| **Prevention** | - Use GPU-based validation<br>- Profile shaders for performance<br>- Implement proper synchronization |

---

### DXGI_ERROR_DEVICE_RESET

| | |
|---|---|
| **Hex Value** | `0x887A0007` |
| **Plain English** | The device failed due to a runtime error and must be recreated |
| **Common Causes** | - Invalid command sequence<br>- Resource state corruption<br>- Memory corruption<br>- Driver detected unrecoverable error |
| **Fix Checklist** | ☐ Enable Debug Layer to catch invalid commands<br>☐ Review command list recording order<br>☐ Check resource state transitions<br>☐ Verify memory operations |
| **Prevention** | Validate all command list operations; use Debug Layer extensively |

---

### DXGI_ERROR_WAS_STILL_DRAWING

| | |
|---|---|
| **Hex Value** | `0x887A000A` |
| **Plain English** | The GPU was busy and couldn't complete the requested operation immediately |
| **Common Causes** | - Calling Present with DXGI_PRESENT_DO_NOT_WAIT while GPU is busy<br>- Trying to access resource while GPU is using it |
| **Fix Checklist** | ☐ Use proper synchronization (fences)<br>☐ Don't use DXGI_PRESENT_DO_NOT_WAIT if you can't handle this error<br>☐ Wait for GPU completion before accessing resources |
| **Prevention** | Implement proper CPU-GPU synchronization |

---

### DXGI_ERROR_FRAME_STATISTICS_DISJOINT

| | |
|---|---|
| **Hex Value** | `0x887A000B` |
| **Plain English** | Frame statistics are unreliable due to a system event |
| **Common Causes** | - System went to sleep/hibernate<br>- Display mode change<br>- Monitor power cycle<br>- Another app took exclusive fullscreen |
| **Fix Checklist** | ☐ Reset frame timing calculations<br>☐ Re-query current statistics<br>☐ Don't use statistics from before this error |
| **Prevention** | Handle gracefully; reset timing state when this occurs |

---

### DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE

| | |
|---|---|
| **Hex Value** | `0x887A000C` |
| **Plain English** | Another application has exclusive ownership of the display output |
| **Common Causes** | - Another game/app is in exclusive fullscreen mode<br>- Screen capture software has exclusive access |
| **Fix Checklist** | ☐ Close other fullscreen applications<br>☐ Use windowed/borderless windowed mode instead<br>☐ Wait and retry |
| **Prevention** | Prefer borderless windowed mode for compatibility |

---

### DXGI_ERROR_DRIVER_INTERNAL_ERROR

| | |
|---|---|
| **Hex Value** | `0x887A0020` |
| **Plain English** | The GPU driver encountered an internal error |
| **Common Causes** | - Driver bug<br>- Driver corruption<br>- Incompatible driver version |
| **Fix Checklist** | ☐ Update to latest GPU drivers<br>☐ Try rolling back to previous driver version<br>☐ Clean install drivers (DDU)<br>☐ Report to GPU vendor if reproducible |
| **Prevention** | Keep drivers updated; test on multiple driver versions |

---

### DXGI_ERROR_NOT_CURRENTLY_AVAILABLE

| | |
|---|---|
| **Hex Value** | `0x887A0022` |
| **Plain English** | The requested resource or operation is temporarily unavailable |
| **Common Causes** | - Resource is in use by another operation<br>- System resources temporarily exhausted<br>- Timing issue |
| **Fix Checklist** | ☐ Retry after a short delay<br>☐ Check if resource is being used elsewhere<br>☐ Implement retry with exponential backoff |
| **Prevention** | Implement retry logic for non-critical operations |

---

### DXGI_ERROR_ACCESS_DENIED

| | |
|---|---|
| **Hex Value** | `0x887A002B` |
| **Plain English** | You don't have permission to perform this operation |
| **Common Causes** | - Writing to a read-only resource<br>- Accessing protected content without rights<br>- Insufficient privileges for operation |
| **Fix Checklist** | ☐ Check resource creation flags<br>☐ Verify you have write access to resource<br>☐ Check content protection requirements |
| **Prevention** | Create resources with appropriate access flags |

---

### DXGI_ERROR_WAIT_TIMEOUT

| | |
|---|---|
| **Hex Value** | `0x887A0027` |
| **Plain English** | A wait operation timed out |
| **Common Causes** | - GPU taking too long<br>- Deadlock in synchronization<br>- GPU crashed but not yet detected |
| **Fix Checklist** | ☐ Increase timeout value<br>☐ Check for GPU hangs<br>☐ Verify synchronization primitives<br>☐ Check if device was removed |
| **Prevention** | Use appropriate timeout values; always check for device removal |

---

### DXGI_ERROR_SDK_COMPONENT_MISSING

| | |
|---|---|
| **Hex Value** | `0x887A002D` |
| **Plain English** | A required SDK component is not installed |
| **Common Causes** | - Debug Layer requested but not installed<br>- Graphics tools not installed<br>- Missing Windows SDK components |
| **Fix Checklist** | ☐ Install "Graphics Tools" optional Windows feature<br>☐ Install Windows SDK<br>☐ Run app without debug features if not available |
| **Prevention** | Check if components exist before enabling debug features |

```cpp
// Safe debug layer initialization:
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    } else {
        OutputDebugStringA("WARNING: Debug layer not available\n");
    }
#endif
```

---

## D3D12 Specific Errors

### D3D12_ERROR_ADAPTER_NOT_FOUND

| | |
|---|---|
| **Hex Value** | `0x887E0001` |
| **Plain English** | The cached PSO was created on a different GPU and can't be reused |
| **Common Causes** | - Loading PSO cache from different machine<br>- GPU was changed since cache was created<br>- Wrong adapter selected for PSO |
| **Fix Checklist** | ☐ Recreate PSO for current adapter<br>☐ Clear PSO cache and rebuild<br>☐ Include adapter identifier in cache key |
| **Prevention** | Tag cached PSOs with adapter info; validate before loading |

---

### D3D12_ERROR_DRIVER_VERSION_MISMATCH

| | |
|---|---|
| **Hex Value** | `0x887E0002` |
| **Plain English** | The cached PSO was created with a different driver version |
| **Common Causes** | - Driver was updated since PSO was cached<br>- Loading cache from system with different driver |
| **Fix Checklist** | ☐ Rebuild PSO cache<br>☐ Include driver version in cache key<br>☐ Implement cache invalidation on driver update |
| **Prevention** | Include driver version in PSO cache metadata |

---

## Common Win32 Errors

### E_FAIL

| | |
|---|---|
| **Hex Value** | `0x80004005` |
| **Plain English** | A general, unspecified error occurred |
| **In DX12 Context** | Often means debug layer enabled but not installed |
| **Fix Checklist** | ☐ Install Graphics Tools Windows feature<br>☐ Check specific API documentation<br>☐ Enable more verbose error reporting |

---

### E_INVALIDARG

| | |
|---|---|
| **Hex Value** | `0x80070057` |
| **Plain English** | One or more arguments are invalid |
| **Common Causes** | - Null pointer where not allowed<br>- Invalid enum value<br>- Size/count mismatch<br>- Invalid flags combination |
| **Fix Checklist** | ☐ Enable Debug Layer for detailed message<br>☐ Validate all parameters<br>☐ Check struct sizes and alignments<br>☐ Verify enum values are valid |

---

### E_OUTOFMEMORY

| | |
|---|---|
| **Hex Value** | `0x8007000E` |
| **Plain English** | Not enough memory to complete the operation |
| **Common Causes** | - GPU memory exhausted<br>- System memory exhausted<br>- Memory fragmentation<br>- Memory leak |
| **Fix Checklist** | ☐ Reduce texture sizes/quality<br>☐ Release unused resources<br>☐ Check for memory leaks<br>☐ Implement memory budget monitoring |
| **Prevention** | Monitor memory usage; implement dynamic quality scaling |

```cpp
// Monitor GPU memory:
DXGI_QUERY_VIDEO_MEMORY_INFO memInfo;
adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);

float usagePercent = (float)memInfo.CurrentUsage / memInfo.Budget * 100.0f;
if (usagePercent > 90.0f) {
    // Approaching memory limit, reduce quality
    ReduceQualitySettings();
}
```

---

### E_NOTIMPL

| | |
|---|---|
| **Hex Value** | `0x80004001` |
| **Plain English** | This method or feature is not implemented |
| **Common Causes** | - Using feature not available on this hardware<br>- Feature removed/deprecated<br>- Calling stub implementation |
| **Fix Checklist** | ☐ Check feature support before using<br>☐ Implement fallback path<br>☐ Review documentation for alternatives |

---

## Error Lookup Utility

### Code Implementation

```cpp
#include <comdef.h>  // For _com_error

void PrintDetailedError(HRESULT hr, const char* context) {
    _com_error err(hr);

    printf("=== DX12 ERROR ===\n");
    printf("Context: %s\n", context);
    printf("HRESULT: 0x%08X\n", hr);
    printf("Message: %ws\n", err.ErrorMessage());

    // Additional DXGI-specific info
    if ((hr & 0xFFFF0000) == 0x887A0000) {
        printf("Type: DXGI Error\n");
        printf("Code: %d (0x%04X)\n", hr & 0xFFFF, hr & 0xFFFF);
    }
    else if ((hr & 0xFFFF0000) == 0x887E0000) {
        printf("Type: D3D12 Error\n");
    }

    printf("==================\n");
}

// Macro for easy use
#define PRINT_ERROR(hr, ctx) PrintDetailedError(hr, ctx)
```

### Quick Reference Table

| Hex Range | Error Type |
|-----------|------------|
| `0x887A0000 - 0x887AFFFF` | DXGI Errors |
| `0x887E0000 - 0x887EFFFF` | D3D12 Errors |
| `0x80070000 - 0x8007FFFF` | Win32 Errors |
| `0x80004000 - 0x80004FFF` | COM Errors |

---

## Troubleshooting Checklists

### "My Game Crashes on Startup"

```
☐ Install latest GPU drivers
☐ Install Visual C++ Redistributable
☐ Install DirectX End-User Runtime
☐ Check minimum system requirements
☐ Run as administrator (for protected paths)
☐ Check Windows Event Viewer for details
☐ Disable overlay software (Discord, GeForce Experience)
☐ Try running with -dx11 flag if available
```

### "Device Removed During Gameplay"

```
☐ Update GPU drivers
☐ Check GPU temperature (use GPU-Z or similar)
☐ Lower graphics settings
☐ Disable GPU overclocking
☐ Check power supply is adequate
☐ Enable DRED and reproduce to get details
☐ Check for known driver issues with your GPU
☐ Try clean driver install with DDU
```

### "Shader Compilation Fails"

```
☐ Check HLSL syntax errors in error message
☐ Verify shader model matches GPU capability
☐ Check for missing #include files
☐ Verify entry point name matches
☐ Check for type mismatches
☐ Ensure DXC is finding dxil.dll
☐ Try compiling with fxc for comparison
```

### "Out of Memory Errors"

```
☐ Lower texture quality
☐ Reduce render resolution
☐ Close background applications
☐ Check for memory leaks (use DXGI debug interface)
☐ Monitor GPU memory with QueryVideoMemoryInfo
☐ Implement streaming/LOD system
☐ Check 32-bit vs 64-bit build
```

### "Black Screen / No Rendering"

```
☐ Check swap chain creation succeeded
☐ Verify render target is properly cleared
☐ Check viewport and scissor rect are set
☐ Verify shaders compiled successfully
☐ Check PSO creation didn't fail silently
☐ Verify root signature matches shader expectations
☐ Check resource states (use PIX to validate)
☐ Enable Debug Layer for validation errors
```

---

## References

### Official Microsoft Documentation

- [Direct3D 12 Return Codes](https://learn.microsoft.com/en-us/windows/win32/direct3d12/d3d12-graphics-reference-returnvalues)
- [DXGI_ERROR Values](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-error)
- [HRESULT Values](https://learn.microsoft.com/en-us/windows/win32/com/structure-of-com-error-codes)
- [Common HRESULT Values](https://learn.microsoft.com/en-us/windows/win32/seccrypto/common-hresult-values)

### Debugging Resources

- [D3D12 Debug Layer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation)
- [DRED Documentation](https://microsoft.github.io/DirectX-Specs/d3d/DeviceRemovedExtendedData.html)
- [PIX for Windows](https://devblogs.microsoft.com/pix/download/)
- [DirectX Debugging Tools](https://github.com/microsoft/DirectX-Debugging-Tools)
