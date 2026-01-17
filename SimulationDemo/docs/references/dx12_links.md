# DirectX 12 Resource Management: Curated Reference Links

> A comprehensive collection of official documentation, tutorials, presentations, and open-source implementations for DX12 resource and memory management.

---

## Table of Contents

1. [Official Microsoft Documentation](#1-official-microsoft-documentation)
2. [Memory & Heap Management](#2-memory--heap-management)
3. [Resource Barriers](#3-resource-barriers)
4. [Descriptor Management](#4-descriptor-management)
5. [Buffer Patterns](#5-buffer-patterns)
6. [Texture Streaming](#6-texture-streaming)
7. [Open-Source Engines & Libraries](#7-open-source-engines--libraries)
8. [GDC & SIGGRAPH Presentations](#8-gdc--siggraph-presentations)
9. [Video Tutorials](#9-video-tutorials)
10. [Community Resources](#10-community-resources)

---

## 1. Official Microsoft Documentation

### Core Documentation

| Resource | Description |
|----------|-------------|
| [DirectX 12 Programming Guide](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide) | Official programming guide covering all D3D12 concepts |
| [DirectX Specs GitHub](https://microsoft.github.io/DirectX-Specs/) | Engineering specifications for DirectX features |
| [D3D12 API Reference](https://learn.microsoft.com/en-us/windows/win32/api/_direct3d12/) | Complete API reference documentation |
| [DirectX Graphics Samples](https://github.com/microsoft/DirectX-Graphics-Samples) | Official sample code from Microsoft |

### Memory Management

| Resource | Description |
|----------|-------------|
| [D3D12_HEAP_TYPE Enumeration](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type) | Official docs for heap types (DEFAULT, UPLOAD, READBACK, CUSTOM) |
| [Residency](https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency) | Memory residency management and budgeting |
| [Suballocation Within Heaps](https://learn.microsoft.com/en-us/windows/win32/direct3d12/suballocation-within-heaps) | Placed resources and sub-allocation strategies |
| [Resource Binding](https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html) | D3D12 resource binding functional spec |

### New Features

| Resource | Description |
|----------|-------------|
| [GPU Upload Heaps (ReBAR)](https://microsoft.github.io/DirectX-Specs/d3d/D3D12GPUUploadHeaps.html) | D3D12_HEAP_TYPE_GPU_UPLOAD specification |
| [Enhanced Barriers](https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html) | New fine-grained barrier API spec |
| [Sampler Feedback](https://microsoft.github.io/DirectX-Specs/d3d/SamplerFeedback.html) | Texture streaming with sampler feedback |
| [Work Graphs](https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html) | GPU-driven work scheduling |

---

## 2. Memory & Heap Management

### Articles & Tutorials

| Resource | Author | Description |
|----------|--------|-------------|
| [Untangling D3D12 Memory Heap Types and Pools](https://asawicki.info/news_1755_untangling_direct3d_12_memory_heap_types_and_pools) | Adam Sawicki | Deep dive into heap types, memory pools, UMA vs NUMA |
| [GPU Memory Pools in D3D12](https://therealmjp.github.io/posts/gpu-memory-pool/) | MJP | Practical memory pooling strategies |
| [Resource Handling in D3D12](https://logins.github.io/graphics/2020/07/31/DX12ResourceHandling.html) | Riccardo Loggini | Comprehensive resource handling guide |
| [DirectX 12 Resources - Key Concepts](https://www.stefanpijnacker.nl/article/directx12-resources-key-concepts/) | Stefan Pijnacker | Resource fundamentals explained |
| [Learning D3D12 from D3D11 - Part 2](https://alextardif.com/D3D11To12P2.html) | Alex Tardif | GPU resources and heaps transition guide |

### AMD GPUOpen

| Resource | Description |
|----------|-------------|
| [D3D12 Memory Allocator (D3D12MA)](https://gpuopen.com/d3d12-memory-allocator/) | AMD's open-source memory allocation library |
| [D3D12MA GitHub](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator) | Source code and documentation |
| [D3D12MA Quick Start](https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/quick_start.html) | Getting started guide |
| [Using D3D12_HEAP_TYPE_GPU_UPLOAD](https://gpuopen.com/learn/using-d3d12-heap-type-gpu-upload/) | ReBAR/SAM usage best practices |

### Memory Budgeting

| Resource | Description |
|----------|-------------|
| [DXGI_QUERY_VIDEO_MEMORY_INFO](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/ns-dxgi1_4-dxgi_query_video_memory_info) | Memory budget query structure |
| [IDXGIAdapter3::QueryVideoMemoryInfo](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiadapter3-queryvideomemoryinfo) | Query current usage and budget |

---

## 3. Resource Barriers

### Official Documentation

| Resource | Description |
|----------|-------------|
| [Using Resource Barriers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12) | Official barrier usage guide |
| [ResourceBarrier Method](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier) | API reference |
| [D3D12_RESOURCE_BARRIER_FLAGS](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags) | Barrier flags (split barriers) |

### Optimization Guides

| Resource | Author | Description |
|----------|--------|-------------|
| [Advanced API Performance: Barriers](https://developer.nvidia.com/blog/advanced-api-performance-barriers/) | NVIDIA | Barrier optimization techniques |
| [First Look at New D3D12 Enhanced Barriers](https://www.asawicki.info/news_1753_first_look_at_new_d3d12_enhanced_barriers) | Adam Sawicki | Enhanced barriers analysis |
| [DirectXTK12 Resource Barriers Wiki](https://github.com/microsoft/DirectXTK12/wiki/Resource-Barriers) | Microsoft | Practical barrier patterns |
| [CPU Efficiency in D3D12](https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html) | Microsoft | CPU-side optimization including barriers |

---

## 4. Descriptor Management

### Descriptor Heaps

| Resource | Author | Description |
|----------|--------|-------------|
| [Managing Descriptor Heaps (Diligent)](https://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/) | Diligent Graphics | Production descriptor heap architecture |
| [[D3D12] Descriptor Heap Strategies](https://www.gamedev.net/forums/topic/686440-d3d12-descriptor-heap-strategies/) | GameDev.net | Community discussion on strategies |
| [Descriptor Tables](https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-tables) | Microsoft | Official descriptor table docs |

### Bindless Rendering

| Resource | Author | Description |
|----------|--------|-------------|
| [Bindless Rendering in DirectX12 and SM6.6](https://rtarun9.github.io/blogs/bindless_rendering/) | Tarun Ramaswamy | Modern bindless implementation |
| [Bindless Rendering Setup (Evolve)](https://www.evolvebenchmark.com/blog-posts/bindless-rendering-setup) | Evolve | Step-by-step bindless setup |
| [Bindless Resources (Alex Tardif)](https://alextardif.com/Bindless.html) | Alex Tardif | Bindless architecture overview |
| [Bindless Descriptors (Wicked Engine)](https://wickedengine.net/2021/04/bindless-descriptors/) | Wicked Engine | Production bindless implementation |
| [[D3D12] Bindless Resources Notes](https://dawnarc.com/2023/04/d3d12bindless-resources-notes/) | Dawnarc | Practical bindless notes |
| [Bindless with DXR (Ray Tracing Gems)](https://link.springer.com/content/pdf/10.1007/978-1-4842-7185-8_17.pdf) | Matt Pettineo | Bindless for ray tracing |

---

## 5. Buffer Patterns

### Constant Buffers

| Resource | Author | Description |
|----------|--------|-------------|
| [Implementing Dynamic Resources](https://diligentgraphics.com/2016/04/20/implementing-dynamic-resources-with-direct3d12/) | Diligent Graphics | Ring buffer implementation |
| [[D3D12] Best Approach to Manage Constant Buffer](https://www.gamedev.net/forums/topic/708811-d3d12-best-approach-to-manage-constant-buffer-for-the-frame/) | GameDev.net | Community strategies |
| [Constant Buffers Tutorial](https://www.braynzarsoft.net/viewtutorial/q16390-directx-12-constant-buffers-root-descriptor-tables) | Braynzar Soft | Beginner-friendly CBV tutorial |
| [[D3D12] Updating Constant Buffer Data](https://www.gamedev.net/forums/topic/670668-d3d12-updating-constant-buffer-data/) | GameDev.net | Update patterns discussion |
| [Getting Started with D3D12](https://anteru.net/blog/2015/getting-started-with-d3d12/) | Anteru | D3D12 fundamentals including buffers |

### Efficient Buffer Management

| Resource | Author | Description |
|----------|--------|-------------|
| [Efficient Buffer Management (NVIDIA GDC)](https://www.gamedevs.org/uploads/efficient-buffer-management.pdf) | John McDonald (NVIDIA) | GDC presentation on buffer strategies |
| [Implementing Dynamic Resources (CodeProject)](https://www.codeproject.com/Articles/1094799/Implementing-Dynamic-Resources-with-Direct-D) | Diligent Graphics | Detailed implementation guide |

---

## 6. Texture Streaming

### Tiled Resources

| Resource | Description |
|----------|-------------|
| [Tiled Resources Overview](https://learn.microsoft.com/en-us/windows/win32/direct3d12/tiled-resources) | Official tiled resources documentation |
| [Volume Tiled Resources](https://learn.microsoft.com/en-us/windows/win32/direct3d12/volume-tiled-resources) | 3D tiled resource support |
| [D3D12_TILED_RESOURCES_TIER](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_tiled_resources_tier) | Hardware tier requirements |

### Streaming Implementations

| Resource | Author | Description |
|----------|--------|-------------|
| [Sampler Feedback Streaming Sample](https://github.com/GameTechDev/SamplerFeedbackStreaming) | Intel | Complete streaming implementation |
| [NVIDIA RTX Texture Streaming (TTM)](https://github.com/NVIDIA-RTX/RTXTS-TTM) | NVIDIA | Tiled texture manager library |
| [D3D12 Resource Management (DeepWiki)](https://deepwiki.com/microsoft/DirectX-Specs/3-d3d12-resource-management) | DeepWiki | Resource management overview |

---

## 7. Open-Source Engines & Libraries

### Production Engines

| Engine | Language | Description | Link |
|--------|----------|-------------|------|
| **The Forge** | C/C++ | Cross-platform rendering framework | [GitHub](https://github.com/ConfettiFX/The-Forge) |
| **Diligent Engine** | C++ | Modern cross-platform graphics engine | [GitHub](https://github.com/DiligentGraphics/DiligentEngine) |
| **Wicked Engine** | C++ | Open-source 3D engine | [GitHub](https://github.com/turanszkij/WickedEngine) |
| **Godot Engine** | C++ | Popular game engine with D3D12 backend | [GitHub](https://github.com/godotengine/godot) |
| **bgfx** | C/C++ | Cross-platform rendering library | [GitHub](https://github.com/bkaradzic/bgfx) |
| **OGRE** | C++ | Object-Oriented Graphics Rendering Engine | [GitHub](https://github.com/OGRECave/ogre) |

### Libraries & Tools

| Library | Description | Link |
|---------|-------------|------|
| **D3D12MA** | AMD memory allocator | [GitHub](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator) |
| **DirectXTK12** | Microsoft helper library | [GitHub](https://github.com/microsoft/DirectXTK12) |
| **DirectX Shader Compiler** | HLSL compiler | [GitHub](https://github.com/microsoft/DirectXShaderCompiler) |
| **D3D12 Agility SDK** | Latest D3D12 features | [NuGet](https://www.nuget.org/packages/Microsoft.Direct3D.D3D12) |
| **PIX** | GPU debugger/profiler | [Microsoft](https://devblogs.microsoft.com/pix/) |
| **RenderDoc** | Graphics debugger | [GitHub](https://github.com/baldurk/renderdoc) |

### Reference Implementations

| Project | Description | Link |
|---------|-------------|------|
| **DirectX-Graphics-Samples** | Official Microsoft samples | [GitHub](https://github.com/microsoft/DirectX-Graphics-Samples) |
| **MiniEngine** | Microsoft's reference engine | [GitHub](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine) |
| **D3D12HelloWorld** | Minimal D3D12 samples | [GitHub](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12HelloWorld) |

---

## 8. GDC & SIGGRAPH Presentations

### GDC Presentations

| Year | Title | Author/Company | Link |
|------|-------|----------------|------|
| 2024 | Advanced Graphics Techniques | Various | [GDC Vault](https://gdcvault.com/) |
| 2023 | Optimizing D3D12 Memory | AMD/NVIDIA | [GDC Vault](https://gdcvault.com/) |
| 2022 | Next-Gen Rendering Techniques | Various | [GDC Vault](https://gdcvault.com/) |
| 2019 | D3D12 Performance Tuning | Microsoft | [GDC Vault](https://gdcvault.com/play/1026191/) |
| 2017 | Getting the Most Out of D3D12 | AMD | [GPUOpen](https://gpuopen.com/presentations/) |
| 2016 | Direct3D 12 New Ways to Maximize Performance | Microsoft | [Channel 9](https://channel9.msdn.com/) |
| 2015 | Direct3D 12 API Preview | Microsoft | [Build](https://channel9.msdn.com/) |

### Key GPU Vendor Presentations

| Title | Company | Link |
|-------|---------|------|
| D3D12 Performance Recommendations | AMD | [GPUOpen Presentations](https://gpuopen.com/presentations/) |
| Advanced API Performance | NVIDIA | [NVIDIA Developer](https://developer.nvidia.com/blog/) |
| Memory Management Best Practices | Intel | [Intel Developer](https://www.intel.com/content/www/us/en/developer/overview.html) |

---

## 9. Video Tutorials

### YouTube Channels

| Channel | Description | Link |
|---------|-------------|------|
| **3Blue1Brown** | Math visualizations (useful for graphics) | [YouTube](https://www.youtube.com/c/3blue1brown) |
| **ChiliTomatoNoodle** | DirectX programming tutorials | [YouTube](https://www.youtube.com/c/ChiliTomatoNoodle) |
| **Arseny Kapoulkine** | Graphics programming deep dives | [YouTube](https://www.youtube.com/user/ArsenyKapoulkine) |
| **javidx9** | Game engine programming | [YouTube](https://www.youtube.com/c/javidx9) |
| **Two Minute Papers** | Research summaries | [YouTube](https://www.youtube.com/c/TwoMinutePapers) |

### Specific Tutorials

| Title | Creator | Description |
|-------|---------|-------------|
| DirectX 12 Ultimate Tutorial Series | Braynzar Soft | Comprehensive D3D12 tutorials |
| D3D12 from Scratch | Various | Multiple beginner series |
| GPU Memory Architecture | GDC Vault | Deep technical talks |

---

## 10. Community Resources

### Forums & Discussions

| Resource | Description | Link |
|----------|-------------|------|
| **GameDev.net** | Graphics programming forums | [GameDev.net](https://www.gamedev.net/forums/forum/technical-forums/graphics-and-gpu-programming/) |
| **r/GraphicsProgramming** | Reddit community | [Reddit](https://www.reddit.com/r/GraphicsProgramming/) |
| **Real-Time Rendering** | Industry blog | [realtimerendering.com](https://www.realtimerendering.com/) |
| **Graphics Programming Discord** | Community chat | [Discord](https://discord.gg/graphicsprogramming) |
| **DirectX Discord** | Microsoft community | [Discord](https://discord.gg/directx) |

### Blogs

| Blog | Author | Description |
|------|--------|-------------|
| [Adam Sawicki's Blog](https://asawicki.info/) | Adam Sawicki | D3D12 deep dives, D3D12MA author |
| [MJP's Blog](https://therealmjp.github.io/) | Matt Pettineo | Advanced graphics techniques |
| [Riccardo Loggini](https://logins.github.io/) | Riccardo Loggini | D3D12 resource handling |
| [Alex Tardif](https://alextardif.com/) | Alex Tardif | Bindless rendering, D3D12 |
| [Diligent Graphics](https://diligentgraphics.com/blog/) | Diligent | Engine architecture |
| [Wicked Engine Blog](https://wickedengine.net/) | Turánszki János | Engine development |
| [IQ's Articles](https://iquilezles.org/articles/) | Inigo Quilez | Shader techniques |
| [Jendrik Illner](https://www.jendrikillner.com/post/) | Jendrik Illner | Weekly graphics news |

### Books

| Title | Author | Description |
|-------|--------|-------------|
| **Introduction to 3D Game Programming with DirectX 12** | Frank Luna | Comprehensive D3D12 textbook |
| **Real-Time Rendering, 4th Edition** | Akenine-Möller et al. | Industry bible |
| **GPU Pro Series** | Wolfgang Engel | Advanced techniques anthology |
| **Ray Tracing Gems I & II** | Various | Ray tracing techniques |
| **Physically Based Rendering** | Pharr, Jakob, Humphreys | PBR theory |

---

## Quick Reference Table

### Heap Types Quick Reference

| Heap Type | CPU Access | GPU Access | Primary Use |
|-----------|------------|------------|-------------|
| DEFAULT | None | Full | Static resources |
| UPLOAD | Write-only | Read | CPU → GPU staging |
| READBACK | Read | Write | GPU → CPU results |
| GPU_UPLOAD | Write (ReBAR) | Full | Dynamic data |
| CUSTOM | Configurable | Configurable | Special cases |

### Resource State Quick Reference

| State | Description |
|-------|-------------|
| COMMON | Default, avoid in hot paths |
| RENDER_TARGET | Render target writes |
| DEPTH_WRITE | Depth buffer writes |
| DEPTH_READ | Depth reads (shadow maps) |
| PIXEL_SHADER_RESOURCE | PS texture reads |
| NON_PIXEL_SHADER_RESOURCE | VS/CS/etc. texture reads |
| UNORDERED_ACCESS | UAV read/write |
| COPY_DEST | Copy destination |
| COPY_SOURCE | Copy source |
| GENERIC_READ | Required for UPLOAD heap |

---

## Version History

| Date | Version | Changes |
|------|---------|---------|
| January 2026 | 1.0 | Initial release |

---

*This document is maintained as part of the ModularGameEngine project.*
*For corrections or additions, please submit a pull request.*
