# PIX for Windows Usage Guide

Comprehensive step-by-step guide for using PIX to debug and profile DirectX 12 applications.

## Table of Contents

1. [Installation & Setup](#installation--setup)
2. [Launching Applications](#launching-applications)
3. [GPU Captures](#gpu-captures)
4. [Timing Captures](#timing-captures)
5. [Shader Debugging](#shader-debugging)
6. [Memory Analysis](#memory-analysis)
7. [TDR Debugging](#tdr-debugging)
8. [Advanced Techniques](#advanced-techniques)
9. [Our Specific Use Cases](#our-specific-use-cases)

---

## Installation & Setup

### Download PIX

1. Visit [PIX Download Page](https://devblogs.microsoft.com/pix/download/)
2. Download the latest version (e.g., PIX 2412.12)
3. Run the installer

### System Requirements

- Windows 10 version 1903+ or Windows 11
- DirectX 12 compatible GPU
- Latest graphics drivers
- Optional: Windows SDK for programmatic integration

### First Launch

```
1. Launch "PIX" from Start Menu
2. Accept any firewall prompts
3. PIX home screen shows recent captures and launch options
```

### Graphics Tools Optional Feature

If PIX reports issues with debug layers:

```
1. Open Settings > System > Optional Features
2. Click "Add a feature"
3. Search "Graphics Tools"
4. Install and restart Windows
```

---

## Launching Applications

### Method 1: Launch Through PIX (Recommended)

```
1. Open PIX
2. Click "Launch Win32" tab
3. Fill in:
   - Path to executable: Browse to your .exe
   - Working directory: Usually same as executable
   - Command line arguments: (optional)
4. Select capture type:
   - "GPU Capture" for graphics debugging
   - "Timing Capture" for performance analysis
5. Click "Launch"
```

### Method 2: Attach to Running Process

For attach support, your application needs to load `WinPixGpuCapturer.dll`:

**Add to your code (reference only):**
```cpp
#ifdef PIX_ENABLE
void LoadPIXGpuCapturer() {
    // Check if PIX is already attached
    if (GetModuleHandleW(L"WinPixGpuCapturer.dll") == nullptr) {
        LoadLibraryW(L"WinPixGpuCapturer.dll");
    }
}
#endif
```

**Then in PIX:**
```
1. Click "Attach" tab
2. Select your process from the list
3. Click "Attach"
```

### Method 3: Programmatic Capture

Add PIX markers and programmatic capture control:

```cpp
// Include PIX header
#include <pix3.h>

// Start programmatic capture
void BeginCapture() {
    PIXCaptureParameters params = {};
    params.GpuCaptureParameters.FileName = L"capture.wpix";
    PIXBeginCapture(PIX_CAPTURE_GPU, &params);
}

void EndCapture() {
    PIXEndCapture(false);  // false = don't discard
}
```

---

## GPU Captures

GPU captures record all D3D12 API calls for a single frame.

### Taking a GPU Capture

**Interactive Method:**
```
1. Launch application through PIX (or attach)
2. Wait until the scene you want to capture appears
3. Press PrintScreen (or F11, or Alt+PrintScreen)
4. Capture appears in PIX Captures panel
5. Double-click capture to analyze
```

**Keyboard Shortcuts:**
| Key | Action |
|-----|--------|
| PrintScreen | Take GPU Capture |
| F11 | Take GPU Capture (alternate) |
| Alt+PrintScreen | Take GPU Capture (alternate) |

### GPU Capture Analysis View

After opening a capture, you'll see:

#### Event List (Left Panel)
```
- Shows all D3D12 API calls in order
- Organized hierarchically by command lists
- Filter by event type
- Search for specific calls
```

#### Timeline (Top)
```
- Visual representation of GPU work
- Shows draw calls, dispatches, barriers
- Click to navigate to specific events
```

#### Pipeline View (Right Panel)
```
- Input Assembler: Vertex/index buffers
- VS/HS/DS/GS/PS stages
- Output Merger: Render targets
- Resource bindings at each stage
```

#### Resource View
```
- Textures: View all bound textures
- Buffers: Inspect buffer contents
- Render Targets: See current output
- Depth Buffer: Visualize depth
```

### Analyzing a Draw Call

```
1. Find draw call in Event List
2. Click to select
3. Examine Pipeline View:
   - Check bound resources
   - Verify shader stages
   - Inspect constant buffers
4. View render target output
5. Compare with expected result
```

### Debugging Missing Geometry

```
Step 1: Find the draw call
- Look for DrawIndexed/Draw calls
- Check if call exists

Step 2: Verify Input Assembler
- Check vertex buffer binding
- Verify index buffer
- Confirm topology

Step 3: Check Transforms
- Inspect constant buffers
- Verify view/projection matrices
- Check world transform

Step 4: Check Culling
- Review rasterizer state
- Check front/back face culling
- Verify scissor rect

Step 5: Check Depth
- View depth buffer
- Verify depth test state
- Check depth write
```

---

## Timing Captures

Timing captures show CPU and GPU performance over time.

### Taking a Timing Capture

```
1. Launch through PIX with "Timing Capture" checked
2. Click the Timing Capture button (clock icon)
3. Let it record for desired duration (1-10 seconds typical)
4. Click stop
5. Capture processes and opens automatically
```

### Timing Analysis Views

#### Timeline View
```
- CPU threads shown as horizontal lanes
- GPU work shown on GPU lane
- Color coded by work type
- Zoom: Mouse wheel
- Pan: Click and drag
```

#### CPU View
```
- Per-thread breakdown
- Function timing
- Call stacks
- Context switches
```

#### GPU View
```
- Pass timing
- Draw call duration
- Shader execution time
- Memory operations
```

### Identifying Bottlenecks

**CPU Bound:**
```
Symptoms:
- GPU timeline has gaps
- CPU threads busy while GPU idle
- Frame time matches CPU work

Actions:
- Look for long CPU functions
- Check for sync points
- Reduce draw calls
```

**GPU Bound:**
```
Symptoms:
- GPU fully utilized
- CPU has idle time
- GPU timeline continuous

Actions:
- Check shader complexity
- Review overdraw
- Optimize memory access
```

---

## Shader Debugging

PIX provides full shader debugging capabilities.

### Debugging a Pixel Shader

```
1. Open GPU capture
2. Navigate to draw call
3. In Pipeline View, click on render target
4. Right-click on a pixel
5. Select "Show Pixel History"
6. Right-click on draw affecting pixel
7. Select "Debug This Pixel"
```

### Pixel History View

Shows all operations that affected a pixel:
```
- Clear operations
- Each draw that touched pixel
- Final pixel value
- Blend operations
```

### Debugging a Vertex Shader

```
1. Navigate to draw call
2. Click on VS Output in Pipeline View
3. Find vertex of interest
4. Right-click vertex
5. Select "Debug This Vertex"
```

### Shader Debug View

Once in debug mode:
```
- View source code with line highlighting
- Step through execution (F10/F11)
- Watch variables
- View registers
- Check resource access
```

### Edit and Continue

Modify shaders without recompiling:

```
1. In Pipeline View, click "Edit" on shader
2. Make changes to HLSL
3. Click "Apply"
4. Frame re-renders with changes
5. Iterate quickly on fixes
```

---

## Memory Analysis

### Viewing Memory Usage

```
1. Open GPU capture
2. View > Memory
3. See all allocated resources
4. Sort by size, type, or name
```

### Resource Details

Click on any resource to see:
```
- Allocation size
- Format
- Dimensions
- Heap type
- Current state
- Usage in frame
```

### Finding Unused Resources

```
1. Open Memory view
2. Look for resources not referenced in Event List
3. Check resource states
4. Identify candidates for cleanup
```

### Texture Inspection

```
1. Find texture in Resources panel
2. Double-click to open viewer
3. View individual mip levels
4. Check all array slices
5. View individual channels (R/G/B/A)
6. Check for corruption patterns
```

---

## TDR Debugging

PIX can help debug GPU timeouts (TDRs).

### Capturing TDRs

```
1. Enable DRED in your application (see DEBUGGING_GUIDE.md)
2. Launch through PIX
3. PIX will capture state when TDR occurs
4. Analyze last frame before crash
```

### GPU Validation

Run validation on captured frame:

```
1. Open GPU capture
2. Click "Run GPU Validation" button
3. Wait for validation to complete
4. Review any errors found
5. Fix issues and re-test
```

### Using Breadcrumbs

Add PIX events for better TDR analysis:

```cpp
// Add markers throughout your code
void RenderShadowPass(ID3D12GraphicsCommandList* cmdList) {
    PIXBeginEvent(cmdList, PIX_COLOR(128, 0, 0), "Shadow Pass");

    for (int i = 0; i < numLights; i++) {
        PIXBeginEvent(cmdList, PIX_COLOR(64, 0, 0), "Light %d", i);
        RenderShadowMap(cmdList, lights[i]);
        PIXEndEvent(cmdList);
    }

    PIXEndEvent(cmdList);
}
```

After TDR, breadcrumbs show last completed operation.

---

## Advanced Techniques

### GPU Capture with Specific Frame

```cpp
// Capture specific frame number
int frameToCapture = 1000;
int currentFrame = 0;

void Render() {
    if (currentFrame == frameToCapture) {
        PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
    }

    // ... render code ...

    if (currentFrame == frameToCapture) {
        PIXEndCapture(false);
    }

    currentFrame++;
}
```

### Conditional Capture

```cpp
// Capture when issue detected
void Render() {
    bool shouldCapture = DetectRenderingAnomaly();

    if (shouldCapture) {
        PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
    }

    // ... render code ...

    if (shouldCapture) {
        PIXEndCapture(false);
    }
}
```

### Named Resources

Help identify resources in PIX:

```cpp
void CreateTexture() {
    device->CreateCommittedResource(..., IID_PPV_ARGS(&texture));

    // Set debug name
    texture->SetName(L"MainColorBuffer");
}

// Resources appear with names in PIX
```

### Comparing Frames

```
1. Capture multiple frames (bad and good)
2. Open both captures
3. Use side-by-side view
4. Compare specific draw calls
5. Identify differences
```

---

## Our Specific Use Cases

### Use Case 1: Debugging Render Pass Issues

When a render pass isn't producing expected output:

```
1. Take GPU capture of problematic frame
2. Navigate to the render pass
3. Check:
   - Render target is bound correctly
   - Clear color is expected
   - Viewport/scissor set properly
   - Correct PSO is set
4. Use Pixel History on unexpected area
5. Debug shader if needed
```

### Use Case 2: Performance Optimization

When frame time is too high:

```
1. Take Timing capture (3-5 seconds)
2. Look for:
   - Longest GPU passes
   - CPU idle time (or vice versa)
   - Sync stalls
3. For GPU bound:
   - Check draw call counts
   - Look for overdraw
   - Review shader complexity
4. For CPU bound:
   - Profile CPU work separately
   - Reduce draw calls
   - Batch more aggressively
```

### Use Case 3: Shadow Map Debugging

When shadows are incorrect:

```
1. Capture frame with shadow issues
2. Find shadow map render pass
3. View shadow map texture:
   - Check depth values
   - Verify coverage
   - Look for precision issues
4. Find shadow sampling pass
5. Debug pixel shader:
   - Check shadow map sampling
   - Verify matrix transforms
   - Check bias values
```

### Use Case 4: Resource State Debugging

When debug layer reports state errors:

```
1. Capture the problematic frame
2. Search for the resource in Event List
3. Track all barriers affecting resource
4. Verify state transitions are correct
5. Check for missing barriers
6. Fix application code
```

### Use Case 5: Post-Processing Debug

When post-process effects are wrong:

```
1. Capture frame
2. Find post-process draw calls
3. For each pass:
   - Verify input textures
   - Check shader constants
   - View output
4. Use Edit & Continue to test fixes
5. Apply fixes to source code
```

---

## PIX Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| PrintScreen | GPU Capture |
| Ctrl+T | Timing Capture |
| F5 | Debug Shader |
| F10 | Step Over (in shader debug) |
| F11 | Step Into (in shader debug) |
| Ctrl+F | Search events |
| Ctrl+G | Go to event |

---

## Troubleshooting PIX

### Cannot Take GPU Capture

```
Possible causes:
1. Application using invalid D3D12 calls
   - Enable debug layer, fix errors first
2. PIX DLL not loaded
   - Launch through PIX instead of attaching
3. Driver issue
   - Update GPU drivers
```

### Capture Won't Replay

```
Possible causes:
1. Different GPU than capture
   - Captures are hardware-specific
2. Driver version mismatch
   - Use same driver version
3. Corruption in capture
   - Try taking a new capture
```

### PIX Crashes

```
Steps:
1. Update PIX to latest version
2. Update GPU drivers
3. Disable GPU overclocking
4. Try with simpler scene
5. Report issue on PIX blog
```

---

## Additional Resources

### Official Documentation

- [PIX Documentation Home](https://devblogs.microsoft.com/pix/documentation/)
- [GPU Captures Guide](https://devblogs.microsoft.com/pix/gpu-captures/)
- [Timing Captures Guide](https://devblogs.microsoft.com/pix/timing-captures/)
- [Taking a Capture](https://devblogs.microsoft.com/pix/taking-a-capture/)

### Release Notes

- [Latest PIX Release](https://devblogs.microsoft.com/pix/pix-2412-12/)
- Check release notes for new features and fixes

### Video Tutorials

- Search YouTube for "PIX DirectX 12 tutorial"
- GDC talks on PIX usage
- Microsoft Channel 9 videos

### Community

- [DirectX Discord](https://discord.gg/directx)
- [PIX Blog Comments](https://devblogs.microsoft.com/pix/)
- [GameDev.net Forums](https://www.gamedev.net/)
