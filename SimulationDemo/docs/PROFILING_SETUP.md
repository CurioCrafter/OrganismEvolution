# Profiling Setup Guide

Complete guide for CPU, GPU, and memory profiling in DirectX 12 applications.

## Table of Contents

1. [Profiling Overview](#profiling-overview)
2. [CPU Profiling](#cpu-profiling)
3. [GPU Profiling](#gpu-profiling)
4. [Memory Profiling](#memory-profiling)
5. [Integrated Profiling](#integrated-profiling)
6. [Interpreting Results](#interpreting-results)

---

## Profiling Overview

### Tool Selection Matrix

| Need | Recommended Tool | Alternative |
|------|-----------------|-------------|
| CPU frame time | Tracy | Superluminal |
| CPU hotspots | Visual Studio Profiler | Intel VTune |
| GPU timing | PIX | NVIDIA Nsight |
| GPU occupancy | Vendor tools (Nsight/RGP) | PIX |
| Memory leaks | D3D12 Debug Layer | PIX |
| Memory usage | PIX | Visual Studio |
| Real-time monitoring | Tracy | Custom HUD |

### Frame Budget Reference

| Target FPS | Frame Budget | CPU Budget | GPU Budget |
|------------|--------------|------------|------------|
| 30 FPS | 33.3 ms | ~25 ms | ~25 ms |
| 60 FPS | 16.6 ms | ~12 ms | ~12 ms |
| 120 FPS | 8.3 ms | ~6 ms | ~6 ms |
| 144 FPS | 6.9 ms | ~5 ms | ~5 ms |

---

## CPU Profiling

### Option 1: Tracy Profiler (Recommended)

Tracy is a real-time, nanosecond resolution frame profiler ideal for game development.

#### Features

- Real-time profiling visualization
- Nanosecond resolution
- Multi-threaded support
- Memory allocation tracking
- Lock contention analysis
- GPU profiling (D3D12 supported)
- Context switch tracking
- Screenshots per frame

#### Installation

```bash
# Clone Tracy repository
git clone https://github.com/wolfpld/tracy.git

# Build the profiler viewer (Windows)
cd tracy/profiler/build/win32
# Open Tracy.sln in Visual Studio and build Release configuration
```

**Download pre-built**: Check [Tracy Releases](https://github.com/wolfpld/tracy/releases) for Windows binaries.

#### Integration

**CMake Setup:**
```cmake
# Add Tracy as subdirectory
add_subdirectory(external/tracy)

# Link to your target
target_link_libraries(YourProject PRIVATE TracyClient)

# Enable Tracy in debug builds
target_compile_definitions(YourProject PRIVATE
    $<$<CONFIG:Debug>:TRACY_ENABLE>
    $<$<CONFIG:RelWithDebInfo>:TRACY_ENABLE>
)
```

**Header Include:**
```cpp
#include <tracy/Tracy.hpp>
```

#### Instrumentation Macros

```cpp
// Mark frame boundaries (required)
void GameLoop() {
    while (running) {
        Update();
        Render();
        FrameMark;  // End of frame
    }
}

// Profile entire function
void Render() {
    ZoneScoped;  // Profiles this function
    // ... rendering code ...
}

// Named zone with color
void Physics() {
    ZoneScopedNC("Physics Update", 0xFF0000);  // Red
    // ... physics code ...
}

// Custom zone
void ProcessEntities() {
    ZoneNamedN(entityZone, "Process Entities", true);
    for (auto& entity : entities) {
        ZoneNamedN(singleEntity, "Single Entity", true);
        entity.Update();
    }
}

// Log messages
void LoadAsset(const char* name) {
    TracyMessageL("Loading asset");
    TracyMessage(name, strlen(name));
}
```

#### GPU Profiling with Tracy

```cpp
// D3D12 GPU context setup
TracyD3D12Context* tracyCtx;

void InitTracyGPU(ID3D12Device* device, ID3D12CommandQueue* queue) {
    tracyCtx = TracyD3D12Context(device, queue);
}

void RenderFrame(ID3D12GraphicsCommandList* cmdList) {
    TracyD3D12Zone(tracyCtx, cmdList, "Render Frame");
    // ... GPU commands ...
}

// Collect GPU data each frame
void EndFrame() {
    TracyD3D12Collect(tracyCtx);
}
```

#### Memory Tracking

```cpp
// Track allocations
void* ptr = malloc(size);
TracyAlloc(ptr, size);

// Track deallocations
TracyFree(ptr);
free(ptr);

// Named allocations
TracyAllocN(ptr, size, "Texture Pool");
TracyFreeN(ptr, "Texture Pool");
```

---

### Option 2: Superluminal

Professional sampling profiler trusted by AAA studios.

#### Key Features

- 8KHz+ sampling rate (vs 1KHz typical)
- Visual timeline UI
- Thread interaction visualization
- Kernel-level call stacks
- No code changes required
- Line-level timing

#### Installation

1. Download from [Superluminal.eu](https://superluminal.eu/)
2. Run installer
3. 14-day free trial available

#### Basic Usage

```
1. Launch Superluminal
2. Click "Run Process" or "Attach to Process"
3. Select your executable
4. Click "Start Recording"
5. Reproduce the scenario
6. Click "Stop" to analyze
```

#### Optional API Integration

```cpp
#include <Superluminal/PerformanceAPI.h>

void GameLoop() {
    PerformanceAPI_BeginEvent("Frame", nullptr, PERFORMANCEAPI_DEFAULT_COLOR);

    Update();
    Render();

    PerformanceAPI_EndEvent();
}
```

---

### Option 3: Visual Studio Profiler

Built-in profiling, no additional installation needed.

#### CPU Usage Tool

```
1. Debug > Performance Profiler (Alt+F2)
2. Select "CPU Usage"
3. Click "Start"
4. Run your application
5. Stop profiling to analyze
```

#### Key Views

- **Call Tree**: Hierarchical function timing
- **Modules**: DLL/EXE breakdown
- **Functions**: Flat list by time
- **Caller/Callee**: Function relationships

#### Instrumentation Profiling

For precise timing (vs sampling):

```
1. Project Properties > C/C++ > Advanced
2. Set "Enable Profiler Information" to Yes
3. Use Performance Profiler with Instrumentation
```

---

### Option 4: Intel VTune

Best for deep CPU architecture analysis.

#### Features

- Microarchitecture analysis
- Cache miss tracking
- Branch misprediction
- Threading analysis

#### Download

[Intel VTune Profiler](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html) (free)

---

## GPU Profiling

### PIX Timing Captures

#### Taking a Timing Capture

```
1. Launch application through PIX
2. Click "Timing Capture" button (or Ctrl+T)
3. Let it run for desired duration
4. Click stop
5. Analyze in timeline view
```

#### What to Look For

- **GPU Duration**: Time spent on GPU work
- **CPU Duration**: Time preparing commands
- **Present Latency**: Frame presentation delay
- **Pipeline Stalls**: Gaps in GPU execution

### NVIDIA Nsight Graphics

Best for NVIDIA GPU optimization.

#### Installation

Download from [NVIDIA Developer](https://developer.nvidia.com/nsight-graphics)

#### Key Features

- GPU Trace: Full frame timeline
- Shader Profiler: Per-instruction timing
- Range Profiler: Custom profiling regions
- Frame Debugger: Full D3D12 support

#### Usage

```
1. Launch Nsight Graphics
2. Connect > Launch Application
3. Set executable and arguments
4. Activity Type: Frame Debugger or GPU Trace
5. Launch
6. Press Ctrl+Z to capture
```

### AMD Radeon GPU Profiler (RGP)

Best for AMD GPU optimization.

#### Installation

Download from [AMD GPUOpen](https://gpuopen.com/rgp/)

#### Integration with PIX

AMD provides a plugin for PIX that adds hardware counters:
- [PIX AMD Plugin](https://gpuopen.com/news/directx12-hardware-counter-profiling-with-microsoft-pix-and-the-amd-plugin/)

#### Key Metrics

- Wavefront occupancy
- Cache hit rates
- Memory bandwidth
- Shader execution time

---

## Memory Profiling

### D3D12 Memory Debugging

#### Enable DXGI Debug Layer

```cpp
void EnableDXGIDebug() {
    ComPtr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
        dxgiDebug->EnableLeakTrackingForThread();
    }
}
```

#### Report Memory Leaks on Shutdown

```cpp
void ReportLiveObjects() {
    ComPtr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
        dxgiDebug->ReportLiveObjects(
            DXGI_DEBUG_ALL,
            DXGI_DEBUG_RLO_FLAGS(
                DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
}
```

### PIX Memory View

PIX provides detailed memory analysis:

```
1. Take a GPU Capture
2. Open capture
3. View > Memory
4. Inspect resource allocations
5. Check for unused resources
```

### Tracking Allocations

#### Custom Allocation Tracking

```cpp
struct AllocationStats {
    std::atomic<size_t> totalAllocated{0};
    std::atomic<size_t> totalFreed{0};
    std::atomic<size_t> allocationCount{0};

    void OnAlloc(size_t size) {
        totalAllocated += size;
        allocationCount++;
    }

    void OnFree(size_t size) {
        totalFreed += size;
        allocationCount--;
    }

    size_t GetCurrentUsage() const {
        return totalAllocated - totalFreed;
    }
};
```

### Resource Heap Analysis

Monitor heap usage:

```cpp
void LogHeapInfo(ID3D12Device* device) {
    D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,
                                &options, sizeof(options));

    // Query adapter memory
    ComPtr<IDXGIAdapter3> adapter;
    // ... get adapter ...

    DXGI_QUERY_VIDEO_MEMORY_INFO localInfo{};
    adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo);

    printf("Local Memory: %llu / %llu MB\n",
           localInfo.CurrentUsage / 1024 / 1024,
           localInfo.Budget / 1024 / 1024);
}
```

---

## Integrated Profiling

### In-Game Profiler HUD

Simple frame time display:

```cpp
class FrameProfiler {
    static constexpr int HISTORY_SIZE = 120;
    float frameHistory[HISTORY_SIZE] = {};
    int historyIndex = 0;

    std::chrono::high_resolution_clock::time_point lastFrame;

public:
    void BeginFrame() {
        lastFrame = std::chrono::high_resolution_clock::now();
    }

    void EndFrame() {
        auto now = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(now - lastFrame).count();

        frameHistory[historyIndex] = ms;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }

    float GetAverageFrameTime() const {
        float sum = 0;
        for (float f : frameHistory) sum += f;
        return sum / HISTORY_SIZE;
    }

    float GetMaxFrameTime() const {
        float max = 0;
        for (float f : frameHistory) if (f > max) max = f;
        return max;
    }
};
```

### GPU Timestamps

Query GPU timing directly:

```cpp
class GPUTimer {
    ComPtr<ID3D12QueryHeap> queryHeap;
    ComPtr<ID3D12Resource> readbackBuffer;
    UINT64 gpuFrequency;

public:
    void Initialize(ID3D12Device* device, ID3D12CommandQueue* queue) {
        queue->GetTimestampFrequency(&gpuFrequency);

        D3D12_QUERY_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        heapDesc.Count = 2;  // Start + End
        device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap));

        // Create readback buffer...
    }

    void BeginTiming(ID3D12GraphicsCommandList* cmdList) {
        cmdList->EndQuery(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    }

    void EndTiming(ID3D12GraphicsCommandList* cmdList) {
        cmdList->EndQuery(queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
        cmdList->ResolveQueryData(queryHeap.Get(),
            D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, readbackBuffer.Get(), 0);
    }

    double GetElapsedMs() {
        UINT64* data;
        readbackBuffer->Map(0, nullptr, (void**)&data);
        double ms = (data[1] - data[0]) * 1000.0 / gpuFrequency;
        readbackBuffer->Unmap(0, nullptr);
        return ms;
    }
};
```

---

## Interpreting Results

### CPU Bottleneck Signs

- GPU utilization < 90% while frame time high
- Large gaps in GPU timeline
- CPU frame time > GPU frame time
- Draw call overhead visible

### GPU Bottleneck Signs

- GPU utilization near 100%
- CPU waiting on GPU (Present stalls)
- Shader execution dominates timeline
- Memory bandwidth saturation

### Common Patterns

#### Pattern 1: CPU Bound

```
Symptoms:
- GPU timeline shows gaps
- CPU profiler shows high activity
- Tracy shows long CPU zones

Solutions:
- Batch draw calls
- Use instancing
- Reduce state changes
- Multi-thread command recording
```

#### Pattern 2: GPU Bound - Shader Heavy

```
Symptoms:
- GPU fully utilized
- Shader execution dominates
- Low occupancy

Solutions:
- Simplify shaders
- Use LOD
- Reduce overdraw
- Optimize texture sampling
```

#### Pattern 3: GPU Bound - Bandwidth

```
Symptoms:
- Memory counters saturated
- Low shader unit utilization

Solutions:
- Compress textures
- Reduce render target size
- Use tiled rendering
- Optimize buffer access patterns
```

#### Pattern 4: Sync Stalls

```
Symptoms:
- Gaps in both CPU and GPU timelines
- High present latency
- Periodic frame spikes

Solutions:
- Double/triple buffer resources
- Use async compute
- Reduce fence waits
- Pipeline frame work
```

---

## Profiling Checklist

### Before Profiling

- [ ] Build in Release/RelWithDebInfo (not Debug)
- [ ] Disable V-Sync for max performance testing
- [ ] Close background applications
- [ ] Use consistent test scenario
- [ ] Run for multiple minutes to warm up

### During Analysis

- [ ] Check both CPU and GPU timelines
- [ ] Look for patterns across multiple frames
- [ ] Identify the longest single operation
- [ ] Check memory usage trends
- [ ] Verify thread utilization

### Performance Targets

- [ ] Frame time within budget
- [ ] No frame time spikes > 2x average
- [ ] GPU utilization > 90% when GPU-bound
- [ ] No memory leaks (stable memory over time)
- [ ] Thread utilization balanced

---

## Additional Resources

### Tracy

- [GitHub Repository](https://github.com/wolfpld/tracy)
- [CppCon 2023 Talk](https://www.youtube.com/watch?v=ghXk3Bk5F2U)
- Documentation PDF included in releases

### Superluminal

- [Official Website](https://superluminal.eu/)
- [Features Overview](https://superluminal.eu/features/)
- [Unity Learn Tutorial](https://learn.unity.com/tutorial/profile-cpu-performance-with-superluminal)

### PIX

- [PIX Documentation](https://devblogs.microsoft.com/pix/documentation/)
- [GPU Captures Guide](https://devblogs.microsoft.com/pix/gpu-captures/)
- [Timing Captures Guide](https://devblogs.microsoft.com/pix/timing-captures/)

### Vendor Tools

- [NVIDIA Nsight Graphics](https://developer.nvidia.com/nsight-graphics)
- [AMD Radeon GPU Profiler](https://gpuopen.com/rgp/)
- [Intel VTune](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html)
