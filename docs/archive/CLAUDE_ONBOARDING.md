# Claude Onboarding and DirectX 12 Orientation

This doc is a short, practical map for AI agents (and humans) who need to make
this project run or diagnose issues quickly. It also gives DirectX 12 API
references and trusted documentation links.

## Reality check (what is real code here)

- This is a DirectX 12 project using HLSL shaders and the Windows graphics stack.
- The active build is `src/main.cpp` (Forge RHI) and is largely monolithic; legacy
  OpenGL sources live under `deprecated/opengl`.
- Shaders are loaded from `Runtime/Shaders/` at runtime (CMake copies them to the
  build output directory).
- `SimulationDemo/` and `docs/` include additional DX12 documentation and design
  notes.

## Quick start references (do not repeat full instructions here)

- Windows build: `BUILD_AND_RUN.md`, `BUILD_INSTRUCTIONS.md`,
  `.claude/BUILD_INSTRUCTIONS.md`
- General setup: `SETUP_GUIDE.md`

## Execution flow (entry points)

- `src/main.cpp` owns window creation, input, timing, simulation update, render
  orchestration, and the ImGui debug panel.
- `ForgeEngine/*` provides the RHI abstraction used by `src/main.cpp`.
- Most other `src/*` modules are not compiled by default; wire them into
  `CMakeLists.txt` if you want to use them.
- Legacy OpenGL paths live under `deprecated/opengl` and
  `deprecated/opengl/legacy_src`.

## Runtime controls and diagnostics

Controls are printed in the console on startup, but the important ones are:

- Camera: WASD movement, mouse look (click to capture), Q/E or Space/C/Ctrl for
  vertical movement, Shift for faster movement.
- Simulation: P toggles pause; speed is adjusted in the ImGui panel.
- Debug: F3 toggles the ImGui debug panel.
- UI: ImGui debug panel only (no in-world dashboard hotkeys).

Diagnostics:

- Console output includes FPS and population stats.
- D3D12 debug layer messages show up in the debugger output window when enabled.

If you see a blank screen:

- Ensure the working directory contains `Runtime/Shaders/` (CMake copies this
  folder next to the executable).
- Check console output for shader compile or pipeline creation failures.
- Enable the D3D12 debug layer to get detailed validation messages.

## DirectX 12 API reference

Key DX12 concepts used in this codebase:

- `ForgeEngine/RHI/*` -> DX12 device, swapchain, command list, and resource wrappers
- `ForgeEngine/Shaders/ShaderCompiler` -> HLSL compilation (DXC)
- `src/main.cpp` -> pipeline creation, constant buffers, and draw calls
- `DrawIndexedInstanced` for instanced creature rendering
- `ClearRenderTargetView` + `ClearDepthStencilView` for frame clearing
- Constant buffers bound via the Forge RHI

For DX12 patterns and implementation details, see:

- `SimulationDemo/docs/RENDERING.md`
- `SimulationDemo/docs/DX12_PIPELINE_GUIDE.md`
- `SimulationDemo/docs/DX12_RESOURCE_MANAGEMENT.md`
- `SimulationDemo/docs/HLSL_BEST_PRACTICES.md`
- `docs/DX12_RENDERING_BEST_PRACTICES.md`

## DirectX 12 references (online)

These are reliable first stops for DX12 topics:

- Microsoft Learn D3D12 docs:
  https://learn.microsoft.com/en-us/windows/win32/direct3d12/
- DirectX Graphics Samples (HelloTriangle, HelloTexture, etc.):
  https://github.com/microsoft/DirectX-Graphics-Samples
- DirectXTK12 (DeviceResources and helper patterns):
  https://github.com/microsoft/DirectXTK12
- D3D12 debug layer guide:
  https://learn.microsoft.com/en-us/windows/win32/direct3d12/d3d12-debug-layer
- PIX for Windows (DX12 GPU capture and profiling):
  https://pix.ms/

## Notes for AI agents

- This is a DirectX 12 project. All graphics code uses the D3D12 API and HLSL
  shaders.
- `src/ai/*`, `src/environment/*`, and other subsystems are not compiled by
  default. Wire them into `CMakeLists.txt` if you want to use them.
- Legacy OpenGL code is in `deprecated/opengl` and should not be used for new
  work.
